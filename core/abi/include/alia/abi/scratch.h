#pragma once
#include <stddef.h>
#include <stdint.h>

// SCRATCH ARENAS

// This file defines the ABI for general-purpose scratch arenas.
//
// Stack-style frames are supported, arenas can optionally be split into
// chunks. They're not thread-safe.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alia_scratch_arena alia_scratch_arena;
typedef struct alia_scratch_marker_def* alia_scratch_marker;

// ALLOCATOR INTERFACE

// represents a chunk of memory allocated by an allocator
typedef struct alia_scratch_chunk_allocation
{
    void* ptr;
    size_t size;
} alia_scratch_chunk_allocation;

// an app-supplied allocator
typedef struct alia_scratch_allocator
{
    void* user; // user pointer for allocator hooks
    alia_scratch_chunk_allocation (*alloc)(
        void* user, size_t existing_capacity, size_t minimum_needed_bytes);
    void (*free)(void* user, alia_scratch_chunk_allocation chunk);
} alia_scratch_allocator;

// OPAQUE CONSTRUCTION/DESTRUCTION

typedef struct alia_struct_spec
{
    size_t size;
    size_t align;
} alia_struct_spec;

alia_struct_spec
alia_scratch_struct_spec(void);

alia_scratch_arena*
alia_scratch_construct(void* mem, alia_scratch_allocator allocator);

void
alia_scratch_destruct(alia_scratch_arena* arena);

// USAGE

void*
alia_scratch_alloc(alia_scratch_arena* arena, size_t bytes, size_t align);

alia_scratch_marker
alia_scratch_mark(alia_scratch_arena* arena);

void
alia_scratch_rewind(alia_scratch_arena* arena, alia_scratch_marker marker);

void
alia_scratch_reset(alia_scratch_arena* arena);

// Trim the scratch arena so that at most `n` chunks remain.
// Note that this should only be called after a full reset.
void
alia_scratch_trim_chunks(alia_scratch_arena* arena, uint32_t n);

// INTROSPECTION

typedef struct alia_scratch_stat_values
{
    size_t bytes_allocated;
    size_t bytes_used;
    uint32_t chunk_count;
} alia_scratch_stat_values;

typedef struct alia_scratch_stats
{
    alia_scratch_stat_values current;
    // Note that peak values don't necessarily correspond to any single point
    // in time. They are independent maximums on their respective fields.
    alia_scratch_stat_values peak;
} alia_scratch_stats;

alia_scratch_stats
alia_scratch_get_stats(alia_scratch_arena const* arena);

#ifdef __cplusplus
}
#endif
