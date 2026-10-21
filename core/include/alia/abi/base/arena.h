#ifndef ALIA_ABI_BASE_ARENA_H
#define ALIA_ABI_BASE_ARENA_H

#include <alia/abi/prelude.h>

#include <stddef.h>
#include <stdint.h>

ALIA_EXTERN_C_BEGIN

// in-place grow callback - `grow` returns the new capacity and must not move
// `base`. A null `grow` makes an overflow a hard failure.
typedef struct alia_grow_callback
{
    void* user;
    size_t (*grow)(
        void* user,
        void* base,
        size_t existing_capacity,
        size_t bytes_requested);
} alia_grow_callback;

static inline alia_grow_callback
alia_grow_callback_none(void)
{
    return ALIA_BRACED_INIT(alia_grow_callback, NULL, NULL);
}

// bump cursor for one pass - The memory system owns the backing.
typedef struct alia_bump_allocator
{
    uint8_t* base;
    size_t capacity;
    size_t offset;
    alia_grow_callback grow;
} alia_bump_allocator;

// Bind a caller-owned buffer. The caller frees `base`.
static inline void
alia_bump_allocator_bind(
    alia_bump_allocator* alloc,
    void* base,
    size_t capacity,
    alia_grow_callback grow)
{
    alloc->base = (uint8_t*) base;
    alloc->capacity = capacity;
    alloc->offset = 0;
    alloc->grow = grow;
}

// Allocation gives offsets from the base, but these helpers can be used to
// convert to/from pointers.

typedef size_t alia_offset;

static inline void*
alia_arena_ptr(alia_bump_allocator const* alloc, alia_offset offset)
{
    return (uint8_t*) alloc->base + offset;
}

static inline alia_offset
alia_arena_offset(alia_bump_allocator const* alloc, void const* ptr)
{
    return (alia_offset) ((uint8_t const*) ptr - (uint8_t const*) alloc->base);
}

// Grow the bump in place.
// If this returns, the bump has enough capacity for the request.
void
alia_arena_handle_out_of_memory(
    alia_bump_allocator* alloc, size_t bytes_requested);

// Allocate bytes from the allocator.
// The requested size must be a multiple of ALIA_MIN_ALIGN.
// Returns an offset from the base of the bump to the allocated bytes.
static inline alia_offset
alia_arena_alloc(alia_bump_allocator* alloc, size_t bytes)
{
    ALIA_ASSERT((bytes & (ALIA_MIN_ALIGN - 1)) == 0);

    if (bytes > alloc->capacity - alloc->offset)
        alia_arena_handle_out_of_memory(alloc, bytes);

    alia_offset offset = alloc->offset;
    alloc->offset += bytes;
    return offset;
}

// Allocate bytes from the allocator, with custom alignment.
alia_offset
alia_arena_alloc_aligned(
    alia_bump_allocator* alloc, size_t bytes, size_t align);

// NAVIGATION

typedef struct alia_arena_marker
{
    size_t offset;
} alia_arena_marker;

static inline alia_arena_marker
alia_arena_mark(alia_bump_allocator* alloc)
{
    return ALIA_BRACED_INIT(alia_arena_marker, alloc->offset);
}

static inline void
alia_arena_jump(alia_bump_allocator* alloc, alia_arena_marker marker)
{
    alloc->offset = marker.offset;
}

// Set the cursor to zero. This does not publish usage.
static inline void
alia_arena_reset(alia_bump_allocator* alloc)
{
    alloc->offset = 0;
}

ALIA_EXTERN_C_END

#endif
