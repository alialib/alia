#ifndef ALIA_KERNEL_ID_H
#define ALIA_KERNEL_ID_H

#include <alia/abi/prelude.h>

#include <alia/abi/base/allocator.h>

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

#define ALIA_MAX_ID_TYPES 256

// vtable for custom, user-defined ID types
typedef struct
{
    // Given two IDs of the same type and size, return true iff they are equal.
    bool (*equals)(void const* a_data, void const* b_data, uint32_t size);

    // Return a 32-bit hash for the given ID.
    // Note that this is only used for internal hash table lookups and is not
    // required to be unique.
    uint32_t (*hash)(void const* data, uint32_t size);

    // Return a clone of the given ID data.
    // Any external data must be allocated with the given allocator.
    void* (*clone)(
        alia_general_allocator* allocator, void const* data, uint32_t size);

    // Destroy the given ID data.
    void (*destroy)(
        alia_general_allocator* allocator, void* data, uint32_t size);
} alia_id_vtable;

// Register a new ID type and return its ID.
// Call this once per custom type. It registers the type's vtable in an
// internal static/global array. The expected use is for the app to call this
// for every type that it defines during static initialization.
uint32_t
alia_id_register_type(alia_id_vtable const* vtable);

// Get the vtable for a registered ID type.
alia_id_vtable const*
alia_id_get_vtable(uint32_t type_id);

#define ALIA_ID_INLINE_CAPACITY 8
#define ALIA_ID_INLINE_FLAG 0x80000000

typedef struct alia_id
{
    uint32_t type_id;
    // top bit: is_inline; lower 31: size (or array count for composite IDs)
    uint32_t size_and_flags;

    union
    {
        uint8_t inline_data[ALIA_ID_INLINE_CAPACITY];
        void const* external_data;
    } payload;
} alia_id;

inline uint32_t
alia_id_get_size(alia_id id)
{
    return id.size_and_flags & ~ALIA_ID_INLINE_FLAG;
}

inline bool
alia_id_is_inline(alia_id id)
{
    return (id.size_and_flags & ALIA_ID_INLINE_FLAG) != 0;
}

inline void const*
alia_id_get_data(alia_id const* id)
{
    return alia_id_is_inline(*id) ? id->payload.inline_data
                                  : id->payload.external_data;
}

bool
alia_id_equals(alia_id a, alia_id b);

uint32_t
alia_id_hash(alia_id id);

alia_id
alia_id_clone(alia_general_allocator* allocator, alia_id id);

void
alia_id_destroy(alia_general_allocator* allocator, alia_id id);

// CONSTRUCTORS FOR BUILT-IN TYPES

inline alia_id
alia_id_make_i32(int32_t val)
{
    alia_id id = {ALIA_ID_TYPE_I32, sizeof(int32_t) | ALIA_ID_INLINE_FLAG};
    memcpy(id.payload.inline_data, &val, sizeof(int32_t));
    return id;
}

inline alia_id
alia_id_make_u32(uint32_t val)
{
    alia_id id = {ALIA_ID_TYPE_U32, sizeof(uint32_t) | ALIA_ID_INLINE_FLAG};
    memcpy(id.payload.inline_data, &val, sizeof(uint32_t));
    return id;
}

inline alia_id
alia_id_make_i64(int64_t val)
{
    alia_id id = {ALIA_ID_TYPE_I64, sizeof(int64_t) | ALIA_ID_INLINE_FLAG};
    memcpy(id.payload.inline_data, &val, sizeof(int64_t));
    return id;
}

inline alia_id
alia_id_make_u64(uint64_t val)
{
    alia_id id = {ALIA_ID_TYPE_U64, sizeof(uint64_t) | ALIA_ID_INLINE_FLAG};
    memcpy(id.payload.inline_data, &val, sizeof(uint64_t));
    return id;
}

inline alia_id
alia_id_make_pointer(void const* ptr)
{
    alia_id id = {ALIA_ID_TYPE_POINTER, sizeof(void*) | ALIA_ID_INLINE_FLAG};
    memcpy(id.payload.inline_data, &ptr, sizeof(void*));
    return id;
}

inline alia_id
alia_id_make_string_literal(char const* static_text)
{
    return alia_id_make_pointer((void const*) static_text);
}

inline alia_id
alia_id_make_bytes(char const* text, uint32_t length)
{
    alia_id id = {ALIA_ID_TYPE_BYTES, length};
    if (length <= ALIA_ID_INLINE_CAPACITY)
    {
        id.size_and_flags |= ALIA_ID_INLINE_FLAG;
        memcpy(id.payload.inline_data, text, length);
    }
    else
    {
        id.payload.external_data = text;
    }
    return id;
}

alia_id
alia_id_make_composite(alia_id const* ids, uint32_t count);

// MATCHERS FOR BUILT-IN TYPES

inline bool
alia_id_matches_i32(alia_id id, int32_t val)
{
    return id.type_id == ALIA_ID_TYPE_I32
        && memcmp(id.payload.inline_data, &val, sizeof(val)) == 0;
}

inline bool
alia_id_matches_u32(alia_id id, uint32_t val)
{
    return id.type_id == ALIA_ID_TYPE_U32
        && memcmp(id.payload.inline_data, &val, sizeof(val)) == 0;
}

inline bool
alia_id_matches_i64(alia_id id, int64_t val)
{
    return id.type_id == ALIA_ID_TYPE_I64
        && memcmp(id.payload.inline_data, &val, sizeof(val)) == 0;
}

inline bool
alia_id_matches_u64(alia_id id, uint64_t val)
{
    return id.type_id == ALIA_ID_TYPE_U64
        && memcmp(id.payload.inline_data, &val, sizeof(val)) == 0;
}

inline bool
alia_id_matches_pointer(alia_id id, void const* ptr)
{
    return id.type_id == ALIA_ID_TYPE_POINTER
        && memcmp(id.payload.inline_data, &ptr, sizeof(ptr)) == 0;
}

inline bool
alia_id_matches_bytes(alia_id id, void const* data, uint32_t length)
{
    return id.type_id == ALIA_ID_TYPE_BYTES && alia_id_get_size(id) == length
        && memcmp(alia_id_get_data(&id), data, length) == 0;
}

ALIA_EXTERN_C_END

#endif // ALIA_KERNEL_ID_H
