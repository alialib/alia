#include <alia/abi/kernel/ids.h>

#include <cstdint>
#include <cstring>

static alia_id_vtable const* alia_type_vtables[ALIA_MAX_ID_TYPES] = {nullptr};

#ifdef ALIA_THREADSAFE
#include <atomic>
static std::atomic<uint32_t> alia_type_count{ALIA_RESERVED_ID_TYPE_COUNT};
#else
static uint32_t alia_type_count = ALIA_RESERVED_ID_TYPE_COUNT;
#endif

static inline void const*
id_view_get_data(alia_id_view const& id)
{
    switch (id.type_id)
    {
        case ALIA_ID_TYPE_NONE:
            return nullptr;
        case ALIA_ID_TYPE_I32:
        case ALIA_ID_TYPE_U32:
        case ALIA_ID_TYPE_I64:
        case ALIA_ID_TYPE_U64:
        case ALIA_ID_TYPE_POINTER:
            return id.payload.inline_data;
        case ALIA_ID_TYPE_BYTES:
            return (id.size_and_flags & ALIA_ID_EXTERNAL_FLAG)
                     ? id.payload.external_data
                     : id.payload.inline_data;
        case ALIA_ID_TYPE_COMPOSITE:
            return id.payload.external_data;
        default:
            return (id.size_and_flags & ALIA_ID_EXTERNAL_FLAG)
                     ? id.payload.external_data
                     : id.payload.inline_data;
    }
}

static inline uint32_t
id_view_get_size(alia_id_view const& id)
{
    return id.size_and_flags & ~ALIA_ID_EXTERNAL_FLAG;
}

static inline bool
id_view_is_external(alia_id_view const& id)
{
    return (id.size_and_flags & ALIA_ID_EXTERNAL_FLAG) != 0;
}

static inline bool
id_view_is_inline(alia_id_view const& id)
{
    return !id_view_is_external(id);
}

static inline void const*
alia_id_view_data_ptr(alia_id_view const& id)
{
    return id_view_get_data(id);
}

static inline uint8_t*
alia_id_view_pool_start(void* mem)
{
    size_t const off
        = alia_align_up(sizeof(alia_captured_id), alignof(alia_id_view));
    return static_cast<uint8_t*>(mem) + off;
}

static alia_id_view
capture_view_into(alia_id_view src, alia_id_capture_context* ctx)
{
    if (src.type_id == ALIA_ID_TYPE_NONE || src.type_id == ALIA_ID_TYPE_I32
        || src.type_id == ALIA_ID_TYPE_U32 || src.type_id == ALIA_ID_TYPE_I64
        || src.type_id == ALIA_ID_TYPE_U64
        || src.type_id == ALIA_ID_TYPE_POINTER)
    {
        return src;
    }

    switch (src.type_id)
    {
        case ALIA_ID_TYPE_BYTES: {
            if (!id_view_is_external(src))
            {
                return src;
            }
            uint32_t const len = id_view_get_size(src);
            void* dst = alia_id_capture_reserve(ctx, len, 1u);
            if (!alia_id_capture_is_discovery(ctx))
                std::memcpy(dst, src.payload.external_data, len);
            src.payload.external_data = dst;
            return src;
        }

        case ALIA_ID_TYPE_COMPOSITE: {
            uint32_t const n = id_view_get_size(src);
            auto const* src_children
                = static_cast<alia_id_view const*>(src.payload.external_data);
            void* arr_mem = alia_id_capture_reserve(
                ctx,
                static_cast<size_t>(n) * sizeof(alia_id_view),
                alignof(alia_id_view));
            auto* dst_children = static_cast<alia_id_view*>(
                alia_id_capture_is_discovery(ctx) ? nullptr : arr_mem);
            for (uint32_t i = 0; i < n; ++i)
            {
                alia_id_view captured_child
                    = capture_view_into(src_children[i], ctx);
                if (dst_children)
                    dst_children[i] = captured_child;
            }
            src.payload.external_data = arr_mem;
            return src;
        }

        default: {
            if (!id_view_is_external(src))
            {
                return src;
            }
            alia_id_vtable const* vt = alia_id_get_vtable(src.type_id);
            ALIA_ASSERT(vt && vt->capture);
            uint32_t const sz = id_view_get_size(src);
            void const* const data = src.payload.external_data;
            src.payload.external_data = vt->capture(ctx, data, sz);
            return src;
        }
    }
}

static void
release_view_in_slab(alia_id_view* v)
{
    if (v->type_id == ALIA_ID_TYPE_NONE)
        return;

    if (id_view_is_inline(*v))
    {
        if (v->type_id >= ALIA_RESERVED_ID_TYPE_COUNT
            && !id_view_is_external(*v))
        {
            alia_id_vtable const* vt = alia_id_get_vtable(v->type_id);
            if (vt && vt->release)
                vt->release(v->payload.inline_data, id_view_get_size(*v));
        }
        return;
    }

    switch (v->type_id)
    {
        case ALIA_ID_TYPE_BYTES:
            break;

        case ALIA_ID_TYPE_COMPOSITE: {
            uint32_t const n = id_view_get_size(*v);
            auto* children = static_cast<alia_id_view*>(const_cast<void*>(
                static_cast<void const*>(v->payload.external_data)));
            for (uint32_t i = 0; i < n; ++i)
                release_view_in_slab(&children[i]);
            break;
        }

        default: {
            if (!id_view_is_external(*v))
                break;
            alia_id_vtable const* vt = alia_id_get_vtable(v->type_id);
            if (vt && vt->release)
            {
                void* p = const_cast<void*>(v->payload.external_data);
                vt->release(p, id_view_get_size(*v));
            }
            break;
        }
    }
}

// fast non-cryptographic mixing finalizer (murmur3 fmix32)
static inline uint32_t
alia_hash_fmix32(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6bU;
    h ^= h >> 13;
    h *= 0xc2b2ae35U;
    h ^= h >> 16;
    return h;
}

static uint32_t
alia_hash_bytes(void const* data, uint32_t size, uint32_t seed)
{
    uint8_t const* bytes = static_cast<uint8_t const*>(data);
    uint32_t h = seed ^ size;
    for (uint32_t i = 0; i < size; ++i)
    {
        h ^= bytes[i];
        h *= 0x01000193U;
    }
    return alia_hash_fmix32(h);
}

static inline uint32_t
alia_hash_u32(uint32_t value, uint32_t seed)
{
    return alia_hash_fmix32(seed ^ value * 0x9e3779b9U);
}

static inline uint32_t
alia_hash_u64(uint64_t value, uint32_t seed)
{
    return alia_hash_u32(
        static_cast<uint32_t>(value) ^ static_cast<uint32_t>(value >> 32),
        seed);
}

static inline uint32_t
alia_hash_combine(uint32_t h, uint32_t item)
{
    return alia_hash_fmix32(h ^ (item + 0x9e3779b9U + (h << 6) + (h >> 2)));
}

extern "C" uint32_t
alia_id_register_type(alia_id_vtable const* vtable)
{
    ALIA_ASSERT(vtable);
    ALIA_ASSERT(vtable->capture);
    uint32_t id;
#ifdef ALIA_THREADSAFE
    id = alia_type_count.fetch_add(1, std::memory_order_relaxed);
#else
    id = alia_type_count++;
#endif
    ALIA_ASSERT(id < ALIA_MAX_ID_TYPES);
    alia_type_vtables[id] = vtable;
    return id;
}

extern "C" alia_id_vtable const*
alia_id_get_vtable(uint32_t type_id)
{
    ALIA_ASSERT(
        type_id >= ALIA_RESERVED_ID_TYPE_COUNT && type_id < ALIA_MAX_ID_TYPES);
    return alia_type_vtables[type_id];
}

extern "C" bool
alia_id_view_equals(alia_id_view a, alia_id_view b)
{
    if (a.type_id != b.type_id || a.size_and_flags != b.size_and_flags)
        return false;

    switch (a.type_id)
    {
        case ALIA_ID_TYPE_NONE:
            return true;

        case ALIA_ID_TYPE_I32:
        case ALIA_ID_TYPE_U32:
            return *reinterpret_cast<const uint32_t*>(a.payload.inline_data)
                == *reinterpret_cast<const uint32_t*>(b.payload.inline_data);

        case ALIA_ID_TYPE_I64:
        case ALIA_ID_TYPE_U64:
            return *reinterpret_cast<const uint64_t*>(a.payload.inline_data)
                == *reinterpret_cast<const uint64_t*>(b.payload.inline_data);

        case ALIA_ID_TYPE_POINTER:
            return *reinterpret_cast<const uintptr_t*>(a.payload.inline_data)
                == *reinterpret_cast<const uintptr_t*>(b.payload.inline_data);

        case ALIA_ID_TYPE_BYTES: {
            uint32_t const size = id_view_get_size(a);
            bool const is_external = id_view_is_external(a);
            void const* a_data = is_external ? a.payload.external_data
                                             : a.payload.inline_data;
            void const* b_data = is_external ? b.payload.external_data
                                             : b.payload.inline_data;
            return std::memcmp(a_data, b_data, size) == 0;
        }

        case ALIA_ID_TYPE_COMPOSITE: {
            uint32_t const count = id_view_get_size(a);
            alia_id_view const* a_arr
                = static_cast<const alia_id_view*>(a.payload.external_data);
            alia_id_view const* b_arr
                = static_cast<const alia_id_view*>(b.payload.external_data);
            for (uint32_t i = 0; i < count; ++i)
            {
                if (!alia_id_view_equals(a_arr[i], b_arr[i]))
                    return false;
            }
            return true;
        }

        default: {
            uint32_t const size = id_view_get_size(a);
            bool const is_external = id_view_is_external(a);
            void const* a_data = is_external ? a.payload.external_data
                                             : a.payload.inline_data;
            void const* b_data = is_external ? b.payload.external_data
                                             : b.payload.inline_data;
            alia_id_vtable const* vt = alia_id_get_vtable(a.type_id);
            ALIA_ASSERT(vt && vt->equals);
            return vt->equals(a_data, b_data, size);
        }
    }
}

extern "C" uint32_t
alia_id_view_hash(alia_id_view id)
{
    uint32_t const size = id_view_get_size(id);
    switch (id.type_id)
    {
        case ALIA_ID_TYPE_NONE:
            return 0u;

        case ALIA_ID_TYPE_I32:
        case ALIA_ID_TYPE_U32: {
            uint32_t value;
            std::memcpy(&value, id.payload.inline_data, sizeof(value));
            return alia_hash_u32(value, 0x5d65b59eU);
        }

        case ALIA_ID_TYPE_I64:
        case ALIA_ID_TYPE_U64: {
            uint64_t value;
            std::memcpy(&value, id.payload.inline_data, sizeof(value));
            return alia_hash_u64(value, 0x1f123bb5U);
        }

        case ALIA_ID_TYPE_POINTER: {
            uintptr_t value;
            std::memcpy(&value, id.payload.inline_data, sizeof(value));
            if (sizeof(uintptr_t) == sizeof(uint64_t))
                return alia_hash_u64(
                    static_cast<uint64_t>(value), 0xbeef31c7U);
            else
                return alia_hash_u32(
                    static_cast<uint32_t>(value), 0xbeef31c7U);
        }

        case ALIA_ID_TYPE_BYTES:
            return alia_hash_bytes(
                alia_id_view_data_ptr(id), size, 0x6a09e667U);

        case ALIA_ID_TYPE_COMPOSITE: {
            uint32_t h = alia_hash_u32(size, 0x3c6ef372U);
            alia_id_view const* items
                = static_cast<alia_id_view const*>(id.payload.external_data);
            for (uint32_t i = 0; i < size; ++i)
                h = alia_hash_combine(h, alia_id_view_hash(items[i]));
            return h;
        }

        default: {
            alia_id_vtable const* vt = alia_id_get_vtable(id.type_id);
            ALIA_ASSERT(vt && vt->hash);
            return alia_hash_combine(
                alia_hash_u32(id.type_id, 0xa54ff53aU),
                vt->hash(alia_id_view_data_ptr(id), size));
        }
    }
}

extern "C" alia_struct_spec
alia_captured_id_spec(alia_id_view root)
{
    if (root.type_id == ALIA_ID_TYPE_NONE)
        return {sizeof(alia_captured_id), alignof(alia_captured_id)};

    size_t const header = sizeof(alia_captured_id);
    size_t const pool0 = alia_align_up(header, alignof(alia_id_view));
    alia_id_capture_context ctx;
    ctx.mode = ALIA_ID_CAPTURE_DISCOVERY;
    ctx.bump = nullptr;
    ctx.discovered_size = pool0;
    ctx.max_align = alignof(alia_captured_id);
    if (alignof(alia_id_view) > ctx.max_align)
        ctx.max_align = alignof(alia_id_view);

    capture_view_into(root, &ctx);

    return {ctx.discovered_size, ctx.max_align};
}

extern "C" void
alia_captured_id_capture_into(alia_id_view root, void* mem, size_t mem_size)
{
    if (root.type_id == ALIA_ID_TYPE_NONE)
    {
        ALIA_ASSERT(mem_size >= sizeof(alia_captured_id));
        *static_cast<alia_captured_id*>(mem) = alia_captured_id_null();
        return;
    }

    alia_struct_spec spec = alia_captured_id_spec(root);
    ALIA_ASSERT(spec.size > 0 && spec.align > 0);
    ALIA_ASSERT(mem_size >= spec.size);
    ALIA_ASSERT((reinterpret_cast<uintptr_t>(mem) & (spec.align - 1)) == 0);

    uint8_t* pool_begin = alia_id_view_pool_start(mem);
    alia_bump_allocator bump = {};
    bump.arena = nullptr;
    bump.base = static_cast<uint8_t*>(mem);
    bump.capacity = mem_size;
    bump.offset = pool_begin - bump.base;
    bump.peak = bump.offset;

    alia_id_capture_context ctx;
    ctx.mode = ALIA_ID_CAPTURE_WRITE;
    ctx.bump = &bump;
    ctx.discovered_size = bump.offset;
    ctx.max_align = spec.align;

    alia_id_view out = capture_view_into(root, &ctx);

    auto* cap = static_cast<alia_captured_id*>(mem);
    cap->view = out;
}

extern "C" void
alia_captured_id_release(void* mem)
{
    auto* cap = static_cast<alia_captured_id*>(mem);
    release_view_in_slab(&cap->view);
}

extern "C" bool
alia_captured_id_matches_view(
    alia_captured_id const* captured, alia_id_view view)
{
    ALIA_ASSERT(captured);
    return alia_id_view_equals(captured->view, view);
}

extern "C" alia_id_view
alia_id_view_make_composite(alia_id_view const* ids, uint32_t count)
{
    alia_id_view id = {ALIA_ID_TYPE_COMPOSITE, count};
    id.payload.external_data = ids;
    return id;
}
