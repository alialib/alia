#ifndef ALIA_KERNEL_ID_H
#define ALIA_KERNEL_ID_H

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>

#include <stdint.h>
#include <string.h>

ALIA_EXTERN_C_BEGIN

// IDs for built-in value types
enum
{
    ALIA_ID_TYPE_NONE,
    ALIA_ID_TYPE_I32,
    ALIA_ID_TYPE_U32,
    ALIA_ID_TYPE_I64,
    ALIA_ID_TYPE_U64,
    ALIA_ID_TYPE_POINTER,
    ALIA_ID_TYPE_BYTES,
    ALIA_ID_TYPE_COMPOSITE,

    ALIA_RESERVED_ID_TYPE_COUNT,
};

#define ALIA_MAX_ID_TYPES 64

typedef enum alia_id_capture_mode
{
    ALIA_ID_CAPTURE_DISCOVERY,
    ALIA_ID_CAPTURE_WRITE,
} alia_id_capture_mode;

// Context for custom-type capture callbacks.
// In discovery mode, reserve helpers only update virtual usage/alignment.
// In write mode, reserve helpers allocate from bump allocator.
typedef struct alia_id_capture_context
{
    alia_id_capture_mode mode;
    alia_bump_allocator* bump;
    size_t discovered_size;
    size_t max_align;
} alia_id_capture_context;

static inline bool
alia_id_capture_is_discovery(alia_id_capture_context const* ctx)
{
    return ctx->mode == ALIA_ID_CAPTURE_DISCOVERY;
}

// Reserve bytes with alignment in either discovery/write mode.
// In discovery mode, no actual memory is touched and NULL is returned.
// In write mode, the returned pointer points inside the caller-owned slab.
static inline void*
alia_id_capture_reserve(
    alia_id_capture_context* ctx, size_t size, size_t align)
{
    ALIA_ASSERT(align >= 1 && (align & (align - 1)) == 0);

    if (ctx->max_align < align)
        ctx->max_align = align;

    size_t const rounded = alia_align_up(size, align);
    // Arena bump allocations are at least ALIA_MIN_ALIGN bytes; keep discovery
    // accounting in sync with what write mode allocates.
    size_t const alloc_size = alia_min_aligned_size(rounded);
    if (alia_id_capture_is_discovery(ctx))
    {
        ctx->discovered_size = alia_align_up(ctx->discovered_size, align);
        ctx->discovered_size += alloc_size;
        return NULL;
    }

    ALIA_ASSERT(ctx->bump);
    ALIA_ASSERT(alloc_size <= (ctx->bump->capacity - ctx->bump->offset));
    alia_offset off = alia_arena_alloc_aligned(ctx->bump, alloc_size, align);
    return alia_arena_ptr(ctx->bump, off);
}

// vtable for custom, user-defined ID types
typedef struct alia_id_vtable
{
    // Given two IDs of the same type and size, return true iff they are equal.
    bool (*equals)(void const* a_data, void const* b_data, uint32_t size);

    // Return a 32-bit hash for the given ID.
    // Note that this is only used for internal hash table lookups and is not
    // required to be unique.
    uint32_t (*hash)(void const* data, uint32_t size);

    // Unified capture callback:
    // - In DISCOVERY mode, callback must reserve all bytes needed via
    //   alia_id_capture_reserve and avoid side effects (e.g. ownership
    //   changes).
    // - In WRITE mode, callback writes captured data and returns the packed
    //   pointer. In discovery mode, callbacks may return NULL.
    void const* (*capture)(
        alia_id_capture_context* ctx, void const* data, uint32_t size);

    // Release non-memory resources for a *captured* payload (refcounts,
    // handles, etc.). Not used for transient views. Slab memory is always
    // freed by the caller; this hook must not free the slab itself.
    void (*release)(void* payload, uint32_t size);
} alia_id_vtable;

// Register a new ID type and return its ID.
uint32_t
alia_id_register_type(alia_id_vtable const* vtable);

// Get the vtable for a registered ID type.
alia_id_vtable const*
alia_id_get_vtable(uint32_t type_id);

#define ALIA_ID_INLINE_CAPACITY 8
// Storage-mode bit for families that use variable/external payloads.
// Note that this is only used in types where it is relevant (i.e., BYTES and
// CUSTOM). Extension-level code may need this for custom constructors, but
// normal callers should construct IDs via the `alia_id_view_make_*` helpers.
#define ALIA_ID_EXTERNAL_FLAG 0x80000000

// borrowed / transient logical ID (may point to data in the stack)
typedef struct alia_id_view
{
    uint32_t type_id;
    // Type-family-dependent metadata (opaque to normal callers):
    // - NONE / fixed-size built-ins: ignored
    // - BYTES / CUSTOM: top bit storage mode (external vs inline), lower bits
    // size
    // - COMPOSITE: lower bits element count (external array implied)
    uint32_t size_and_flags;

    union
    {
        uint8_t inline_data[ALIA_ID_INLINE_CAPACITY];
        void const* external_data;
    } payload;
} alia_id_view;

// stable captured identity - wraps a view whose external payloads (if any)
// live entirely inside the same caller-owned slab as this struct
typedef struct alia_captured_id
{
    alia_id_view view;
} alia_captured_id;

static inline alia_id_view const*
alia_captured_id_as_view(alia_captured_id const* id)
{
    return &id->view;
}

static inline alia_id_view
alia_id_view_null(void)
{
    return ALIA_BRACED_INIT(alia_id_view, 0);
}

static inline bool
alia_id_view_is_null(alia_id_view id)
{
    return id.type_id == ALIA_ID_TYPE_NONE;
}

static inline alia_captured_id
alia_captured_id_null(void)
{
    return ALIA_BRACED_INIT(alia_captured_id, 0);
}

static inline bool
alia_captured_id_is_null(alia_captured_id const* id)
{
    return id->view.type_id == ALIA_ID_TYPE_NONE;
}

bool
alia_id_view_equals(alia_id_view a, alia_id_view b);

uint32_t
alia_id_view_hash(alia_id_view id);

// Total size and alignment for a single slab that can hold `root` once
// captured (header `alia_captured_id` plus internal tail storage).
alia_struct_spec
alia_captured_id_spec(alia_id_view root);

// Copy transient `root` into `mem`..`mem+mem_size` as an `alia_captured_id`
// at offset 0. Preconditions: `mem` aligned to `spec.align`, `mem_size >=
// spec.size`, no overlap between `mem` and transient external payloads.
void
alia_captured_id_capture_into(alia_id_view root, void* mem, size_t mem_size);

// Run `release` hooks for resources held by a captured ID inside `mem` (same
// pointer passed to `capture_into`). Does not free `mem`.
void
alia_captured_id_release(void* mem);

bool
alia_captured_id_matches_view(
    alia_captured_id const* captured, alia_id_view view);

// --- Built-in constructors (return transient views) ---

static inline alia_id_view
alia_id_view_make_i32(int32_t val)
{
    alia_id_view id = {ALIA_ID_TYPE_I32, 0};
    memcpy(id.payload.inline_data, &val, sizeof(int32_t));
    return id;
}

static inline alia_id_view
alia_id_view_make_u32(uint32_t val)
{
    alia_id_view id = {ALIA_ID_TYPE_U32, 0};
    memcpy(id.payload.inline_data, &val, sizeof(uint32_t));
    return id;
}

static inline alia_id_view
alia_id_view_make_i64(int64_t val)
{
    alia_id_view id = {ALIA_ID_TYPE_I64, 0};
    memcpy(id.payload.inline_data, &val, sizeof(int64_t));
    return id;
}

static inline alia_id_view
alia_id_view_make_u64(uint64_t val)
{
    alia_id_view id = {ALIA_ID_TYPE_U64, 0};
    memcpy(id.payload.inline_data, &val, sizeof(uint64_t));
    return id;
}

static inline alia_id_view
alia_id_view_make_pointer(void const* ptr)
{
    alia_id_view id = {ALIA_ID_TYPE_POINTER, 0};
    memcpy(id.payload.inline_data, &ptr, sizeof(void*));
    return id;
}

static inline alia_id_view
alia_id_view_make_string_literal(char const* static_text)
{
    return alia_id_view_make_pointer((void const*) static_text);
}

static inline alia_id_view
alia_id_view_make_bytes(char const* text, uint32_t length)
{
    alia_id_view id = {ALIA_ID_TYPE_BYTES, length};
    if (length <= ALIA_ID_INLINE_CAPACITY)
    {
        memcpy(id.payload.inline_data, text, length);
    }
    else
    {
        id.size_and_flags |= ALIA_ID_EXTERNAL_FLAG;
        id.payload.external_data = text;
    }
    return id;
}

// Construct an external custom ID view.
// `type_id` must be a registered custom type ID.
static inline alia_id_view
alia_id_view_make_custom_external(
    uint32_t type_id, void const* data, uint32_t size)
{
    alia_id_view id = {type_id, size | ALIA_ID_EXTERNAL_FLAG};
    id.payload.external_data = data;
    return id;
}

// Construct an inline custom ID view.
// `type_id` must be a registered custom type ID.
static inline alia_id_view
alia_id_view_make_custom_inline(
    uint32_t type_id, void const* data, uint32_t size)
{
    ALIA_ASSERT(size <= ALIA_ID_INLINE_CAPACITY);
    alia_id_view id = {type_id, size};
    memcpy(id.payload.inline_data, data, size);
    return id;
}

alia_id_view
alia_id_view_make_composite(alia_id_view const* ids, uint32_t count);

ALIA_EXTERN_C_END

#endif // ALIA_KERNEL_ID_H
