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
typedef struct alia_frame_hdr* alia_frame_handle;

// CONFIGURATION

typedef struct alia_scratch_chunk_allocation
{
    void* ptr;
    size_t size;
} alia_scratch_chunk_allocation;

typedef struct alia_scratch_config
{
    size_t hard_cap_bytes; // 0 => no hard cap (abort if exceeded)
    void* user; // user pointer for allocator hooks
    alia_scratch_chunk_allocation (*alloc)(
        void* user, size_t existing_capacity, size_t minimum_needed_bytes);
    void (*free)(void* user, alia_scratch_chunk_allocation chunk);
} alia_scratch_config;

// STATISTICS

typedef struct alia_scratch_stats
{
    size_t committed_bytes; // sum of all chunk payloads
    size_t peak_used_bytes; // approximate high-water usage
    uint32_t max_chunks_touched; // max chunks touched in last pass
} alia_scratch_stats;

// CREATION AND DESTRUCTION (HEAP-OWNED)

alia_scratch_arena*
alia_scratch_create(const alia_scratch_config* cfg);
void
alia_scratch_destroy(alia_scratch_arena* arena);

// CONSTRUCTION AND DESTRUCTION (CALLER-SUPPLIED STORAGE)

size_t
alia_scratch_state_size(void);
size_t
alia_scratch_state_align(void);
alia_scratch_arena*
alia_scratch_construct(void* mem, const alia_scratch_config* cfg);
void
alia_scratch_destruct(alia_scratch_arena* arena);

// PASS LIFECYCLE

alia_frame_handle
alia_scratch_begin_pass(alia_scratch_arena* arena);
void
alia_scratch_end_pass(alia_scratch_arena* arena);

// TRIMMING

// Trim the scratch arena so that at most `n` chunks remain.
void
alia_scratch_trim_chunks(alia_scratch_arena* arena, uint32_t n);

// ALLOCATION

void*
alia_scratch_alloc(alia_scratch_arena* arena, size_t bytes, size_t align);

// FRAMES

alia_frame_handle
alia_push_frame(alia_scratch_arena* arena);
void
alia_pop_frame(alia_scratch_arena* arena, alia_frame_handle h);

// STATISTICS ACCESS

alia_scratch_stats
alia_scratch_get_stats(const alia_scratch_arena* arena);

#ifdef __cplusplus
}
#endif
