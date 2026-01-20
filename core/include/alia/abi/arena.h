#pragma once

#include <alia/abi/base.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alia_arena alia_arena;

typedef struct alia_bump_state
{
    uint8_t* base;
    size_t capacity;
    size_t offset;
} alia_bump_state;

typedef alia_bump_state alia_arena_view;

// a callback that can be used to grow the arena (in place)
typedef size_t (*alia_arena_grow_fn)(
    void* user, void* base, size_t existing_capacity, size_t bytes_requested);

// a callback that can be used to free the arena
typedef void (*alia_arena_free_fn)(void* user, void* base, size_t capacity);

typedef struct alia_arena_controller
{
    void* user; // user-provided data
    alia_arena_grow_fn grow;
    alia_arena_free_fn free;
} alia_arena_controller;

static inline alia_arena_controller
alia_arena_no_controller(void)
{
    return alia_arena_controller{.user = NULL, .grow = NULL, .free = NULL};
}

// LIFECYCLE

alia_struct_spec
alia_arena_object_spec(void);

alia_arena*
alia_arena_init(
    void* object_storage,
    void* buffer,
    size_t capacity,
    alia_arena_controller controller);

void
alia_arena_cleanup(alia_arena* arena);

// ALLOCATION

alia_arena_view*
alia_arena_get_view(alia_arena* arena);

// Allocation gives offsets from the base, but these helpers can be used to
// convert to/from pointers.

typedef size_t alia_offset;

static inline void*
alia_arena_ptr(alia_arena_view const* arena, alia_offset offset)
{
    return (uint8_t*) arena->base + offset;
}

static inline alia_offset
alia_arena_offset(alia_arena_view const* arena, void const* ptr)
{
    return (alia_offset) ((uint8_t const*) ptr - (uint8_t const*) arena->base);
}

// Handle out-of-memory errors.
// If this returns, the arena has sufficient capacity to satisfy the request.
void
alia_arena_handle_out_of_memory(
    alia_arena_view* arena, size_t bytes_requested);

// Allocate bytes from the arena.
// The requested size must be a multiple of ALIA_MIN_ALIGN.
// Returns an offset from the base of the arena to the allocated bytes.
static inline alia_offset
alia_arena_alloc(alia_arena_view* arena, size_t bytes)
{
    ALIA_ASSERT((bytes & (ALIA_MIN_ALIGN - 1)) == 0);

    if (bytes > arena->capacity - arena->offset)
        alia_arena_handle_out_of_memory(arena, bytes);

    alia_offset offset = arena->offset;
    arena->offset += bytes;
    return offset;
}

// Allocate bytes from the arena, with custom alignment.
alia_offset
alia_arena_alloc_aligned(alia_arena_view* arena, size_t bytes, size_t align);

// NAVIGATION

// Update the peak usage of the arena.
// This is used internally to update the arena stats before rewinding.
void
alia_update_peak_usage(alia_arena_view* view);

struct alia_arena_marker
{
    size_t offset;
};

static inline alia_arena_marker
alia_arena_mark(alia_arena_view* arena)
{
    return alia_arena_marker{.offset = arena->offset};
}

static inline void
alia_arena_jump(alia_arena_view* arena, alia_arena_marker marker)
{
    alia_update_peak_usage(arena);
    arena->offset = marker.offset;
}

static inline void
alia_arena_reset(alia_arena_view* arena)
{
    alia_update_peak_usage(arena);
    arena->offset = 0;
}

// INTROSPECTION

typedef struct alia_arena_stats
{
    size_t current_usage;
    size_t peak_usage;
} alia_arena_stats;

alia_arena_stats
alia_arena_get_stats(alia_arena* view);

#ifdef __cplusplus
}
#endif
