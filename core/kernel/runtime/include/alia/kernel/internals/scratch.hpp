#pragma once

#include <alia/scratch.h>

#include <alia/kernel/base.hpp>

// INTERNAL STRUCTURES

namespace alia {

struct scratch_chunk
{
    // the original allocation result from the allocator
    alia_scratch_chunk_allocation alloc;
    // next chunk in the list
    scratch_chunk* next = nullptr;
};

struct scratch_bump_state
{
    std::uint8_t* head = nullptr;
    std::uint8_t* end = nullptr;
};

} // namespace alia

// ABI STRUCTURES

struct alia_scratch_marker_def
{
    std::size_t bytes_used = 0;
    alia::scratch_chunk* chunk = nullptr;
    alia::scratch_bump_state bump{};
};

struct alia_scratch_arena
{
    // user-provided allocator
    alia_scratch_allocator allocator{};

    // chunk list
    alia::scratch_chunk* first_chunk = nullptr;
    alia::scratch_chunk* active_chunk = nullptr;

    // bump allocator state
    alia::scratch_bump_state bump{};

    // stats - Note that peak values aren't updated until requested, and
    // `stats.current.bytes_used` only reflects chunks that have been advanced
    // past.
    alia_scratch_stats stats{};
};

// INTERNAL API

namespace alia {

void
activate_new_chunk(alia_scratch_arena* arena, std::size_t bytes_required);

inline void*
scratch_alloc(alia_scratch_arena* arena, std::size_t bytes, std::size_t align)
{
    std::uint8_t* p = align_ptr(arena->bump.head, align);
    std::uint8_t* next = p + bytes;
    if (next > arena->bump.end)
    {
        activate_new_chunk(arena, bytes + align);
        p = align_ptr(arena->bump.head, align);
        next = p + bytes;
    }
    arena->bump.head = next;
    return p;
}

} // namespace alia
