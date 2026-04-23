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
alia_id_data_ptr(alia_id const& id)
{
    return alia_id_is_inline(id) ? id.payload.inline_data : id.payload.external_data;
}

// Fast non-cryptographic mixing finalizer (Murmur3 fmix32).
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
        static_cast<uint32_t>(value) ^ static_cast<uint32_t>(value >> 32), seed);
}

static inline uint32_t
alia_hash_combine(uint32_t h, uint32_t item)
{
    return alia_hash_fmix32(h ^ (item + 0x9e3779b9U + (h << 6) + (h >> 2)));
}

extern "C" uint32_t
alia_id_register_type(alia_id_vtable const* vtable)
{
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
alia_id_equals(alia_id a, alia_id b)
{
    if (a.type_id != b.type_id || a.size_and_flags != b.size_and_flags)
        return false;

    switch (a.type_id)
    {
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
            // Dynamic strings: Only here do we actually pay for the
            // inline/external check
            uint32_t const size = alia_id_get_size(a);
            bool const is_inline = alia_id_is_inline(a);
            void const* a_data
                = is_inline ? a.payload.inline_data : a.payload.external_data;
            void const* b_data
                = is_inline ? b.payload.inline_data : b.payload.external_data;
            return std::memcmp(a_data, b_data, size) == 0;
        }

        case ALIA_ID_TYPE_COMPOSITE: {
            uint32_t const count = alia_id_get_size(a);
            alia_id const* a_arr
                = static_cast<const alia_id*>(a.payload.external_data);
            alia_id const* b_arr
                = static_cast<const alia_id*>(b.payload.external_data);
            for (uint32_t i = 0; i < count; ++i)
            {
                if (!alia_id_equals(a_arr[i], b_arr[i]))
                    return false;
            }
            return true;
        }

        default: {
            uint32_t const size = alia_id_get_size(a);
            bool const is_inline = alia_id_is_inline(a);
            void const* a_data
                = is_inline ? a.payload.inline_data : a.payload.external_data;
            void const* b_data
                = is_inline ? b.payload.inline_data : b.payload.external_data;
            alia_id_vtable const* vt = alia_id_get_vtable(a.type_id);
            ALIA_ASSERT(vt && vt->equals);
            return vt->equals(a_data, b_data, size);
        }
    }
}

extern "C" uint32_t
alia_id_hash(alia_id id)
{
    uint32_t const size = alia_id_get_size(id);
    switch (id.type_id)
    {
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
                return alia_hash_u64(static_cast<uint64_t>(value), 0xbeef31c7U);
            else
                return alia_hash_u32(static_cast<uint32_t>(value), 0xbeef31c7U);
        }

        case ALIA_ID_TYPE_BYTES:
            return alia_hash_bytes(alia_id_data_ptr(id), size, 0x6a09e667U);

        case ALIA_ID_TYPE_COMPOSITE: {
            uint32_t h = alia_hash_u32(size, 0x3c6ef372U);
            alia_id const* items = static_cast<alia_id const*>(id.payload.external_data);
            for (uint32_t i = 0; i < size; ++i)
                h = alia_hash_combine(h, alia_id_hash(items[i]));
            return h;
        }

        default: {
            alia_id_vtable const* vt = alia_id_get_vtable(id.type_id);
            ALIA_ASSERT(vt && vt->hash);
            return alia_hash_combine(
                alia_hash_u32(id.type_id, 0xa54ff53aU),
                vt->hash(alia_id_data_ptr(id), size));
        }
    }
}

extern "C" alia_id
alia_id_clone(alia_general_allocator* allocator, alia_id id)
{
    if (alia_id_is_inline(id))
    {
        // There's nothing to clone.
        // Note that this is guaranteed to cover all built-in types other than
        // ALIA_ID_TYPE_BYTES and ALIA_ID_TYPE_COMPOSITE.
        return id;
    }

    uint32_t const size = alia_id_get_size(id);

    alia_id cloned = id;

    switch (id.type_id)
    {
        case ALIA_ID_TYPE_BYTES: {
            void* persistent_data = allocator->alloc(allocator->user_data, size, 8);
            std::memcpy(persistent_data, id.payload.external_data, size);
            cloned.payload.external_data = persistent_data;
            break;
        }

        case ALIA_ID_TYPE_COMPOSITE: {
            alia_id const* transient_array
                = static_cast<alia_id const*>(id.payload.external_data);
            alia_id* persistent_array = static_cast<alia_id*>(allocator->alloc(
                allocator->user_data,
                size * sizeof(alia_id),
                alignof(alia_id)));
            for (uint32_t i = 0; i < size; ++i)
            {
                persistent_array[i]
                    = alia_id_clone(allocator, transient_array[i]);
            }
            cloned.payload.external_data = persistent_array;
            break;
        }

        default: {
            alia_id_vtable const* vt = alia_id_get_vtable(id.type_id);
            ALIA_ASSERT(vt && vt->clone);
            void* persistent_data
                = vt->clone(allocator, id.payload.external_data, size);
            cloned.payload.external_data = persistent_data;
            break;
        }
    }

    return cloned;
}

extern "C" alia_id
alia_id_make_composite(alia_id const* ids, uint32_t count)
{
    alia_id id = {ALIA_ID_TYPE_COMPOSITE, count};
    id.payload.external_data = ids;
    return id;
}

extern "C" void
alia_id_destroy(alia_general_allocator* allocator, alia_id id)
{
    if (alia_id_is_inline(id))
    {
        // There's nothing to destroy.
        return;
    }

    switch (id.type_id)
    {
        case ALIA_ID_TYPE_BYTES: {
            allocator->free(
                allocator->user_data,
                const_cast<void*>(id.payload.external_data),
                alia_id_get_size(id),
                8);
            break;
        }

        case ALIA_ID_TYPE_COMPOSITE: {
            alia_id const* array
                = static_cast<alia_id const*>(id.payload.external_data);
            uint32_t const size = alia_id_get_size(id);
            for (uint32_t i = 0; i < size; ++i)
            {
                alia_id_destroy(allocator, array[i]);
            }
            allocator->free(
                allocator->user_data,
                const_cast<void*>(id.payload.external_data),
                size * sizeof(alia_id),
                alignof(alia_id));
            break;
        }

        default: {
            alia_id_vtable const* vt = alia_id_get_vtable(id.type_id);
            ALIA_ASSERT(vt && vt->destroy);
            vt->destroy(
                allocator,
                const_cast<void*>(id.payload.external_data),
                alia_id_get_size(id));
            break;
        }
    }
}
