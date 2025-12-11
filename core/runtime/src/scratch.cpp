#include <alia/scratch.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <alia/base.hpp>
#include <alia/internals/scratch.hpp>

namespace alia {

// INTERNAL FUNCTIONS

std::uint8_t*
chunk_start(scratch_chunk* c)
{
    return reinterpret_cast<std::uint8_t*>(c + 1);
}
std::uint8_t*
chunk_end(scratch_chunk* c)
{
    return reinterpret_cast<std::uint8_t*>(c->alloc.ptr) + c->alloc.size;
}
std::size_t
chunk_capacity(scratch_chunk* c)
{
    return chunk_end(c) - chunk_start(c);
}

void
record_bytes_used(alia_scratch_arena* arena)
{
    if (arena->active_chunk)
    {
        arena->stats.current.bytes_used
            += arena->bump.head - chunk_start(arena->active_chunk);
    }
}

void
activate_chunk(alia_scratch_arena* arena, scratch_chunk* chunk)
{
    arena->active_chunk = chunk;
    arena->bump.head = chunk_start(chunk);
    arena->bump.end = chunk_end(chunk);
}

void
update_peak_allocation_stats(alia_scratch_arena* arena)
{
    auto& stats = arena->stats;
    if (stats.current.bytes_allocated > stats.peak.bytes_allocated)
        stats.peak.bytes_allocated = stats.current.bytes_allocated;
    if (stats.current.chunk_count > stats.peak.chunk_count)
        stats.peak.chunk_count = stats.current.chunk_count;
}

void
update_peak_usage_stats(alia_scratch_arena* arena)
{
    auto& stats = arena->stats;
    if (stats.current.bytes_used > stats.peak.bytes_used)
        stats.peak.bytes_used = stats.current.bytes_used;
}

void
activate_new_chunk(alia_scratch_arena* arena, std::size_t bytes_required)
{
    // We're moving past the active chunk, so record its used bytes.
    record_bytes_used(arena);

    // If we're not already at the end of the chunk list, walk through the list
    // to see if we can find a chunk that has enough space.
    scratch_chunk* chunk = arena->active_chunk;
    if (chunk)
    {
        while (chunk->next)
        {
            chunk = chunk->next;
            if (chunk_capacity(chunk) >= bytes_required)
            {
                activate_chunk(arena, chunk);
                return;
            }
        }
    }
    // Otherwise, we need to allocate a new chunk...
    // Note that at this point, `chunk` is either null (if there were no chunks
    // yet) or the last chunk in the list.

    // Request at least enough space for the chunk header, alignment padding,
    // and the actual request. (Note that `bytes_required` already includes
    // alignment padding for the actual request.)
    const std::size_t bytes_to_request
        = sizeof(scratch_chunk) + alignof(scratch_chunk) + bytes_required;

    // Invoke the user allocator and create the new chunk structure.
    alia_scratch_chunk_allocation alloc = (arena->allocator.alloc)(
        arena->allocator.user,
        arena->stats.current.bytes_allocated,
        bytes_to_request);
    auto* new_chunk = static_cast<scratch_chunk*>(
        align_ptr(alloc.ptr, alignof(scratch_chunk)));
    new_chunk->alloc = alloc;
    new_chunk->next = nullptr;

    // Append it to the arena's chunk list.
    if (chunk)
        chunk->next = new_chunk;
    else
        arena->first_chunk = new_chunk;

    // Update allocation stats.
    arena->stats.current.bytes_allocated += alloc.size;
    ++arena->stats.current.chunk_count;
    update_peak_allocation_stats(arena);

    activate_chunk(arena, new_chunk);
}

void
trim_chunks(alia_scratch_arena* arena, std::uint32_t chunk_count_to_keep)
{
    // Advance past the chunks that we're keeping, counting the total number of
    // bytes allocated among them.
    std::size_t total_kept_bytes = 0;
    scratch_chunk* chunk = arena->first_chunk;
    std::uint32_t chunks_touched = 0;
    scratch_chunk* previous_chunk = nullptr;
    while (chunk && chunks_touched < chunk_count_to_keep)
    {
        total_kept_bytes += chunk->alloc.size;
        previous_chunk = chunk;
        chunk = chunk->next;
        ++chunks_touched;
    }
    // If the chunk list is shorter than the number to keep, there's nothing to
    // do.
    if (chunks_touched < chunk_count_to_keep)
        return;

    // Free the rest of the chunks.
    while (chunk)
    {
        auto* next = chunk->next;
        (arena->allocator.free)(arena->allocator.user, chunk->alloc);
        chunk = next;
    }
    // Terminate the chunk list.
    if (previous_chunk)
        previous_chunk->next = nullptr;

    // Update stats.
    arena->stats.current.bytes_allocated = total_kept_bytes;
    arena->stats.current.chunk_count = chunk_count_to_keep;
}

} // namespace alia

// ABI FUNCTIONS

using namespace alia;

extern "C" {

alia_struct_spec
alia_scratch_struct_spec(void)
{
    return {sizeof(alia_scratch_arena), alignof(alia_scratch_arena)};
}

alia_scratch_arena*
alia_scratch_construct(void* mem, alia_scratch_allocator allocator)
{
    auto* A = new (mem) alia_scratch_arena{};
    A->allocator = allocator;
    return A;
}

void
alia_scratch_destruct(alia_scratch_arena* arena)
{
    if (!arena)
        return;

    // Free all chunks.
    auto* chunk = arena->first_chunk;
    while (chunk)
    {
        auto* next = chunk->next;
        (arena->allocator.free)(arena->allocator.user, chunk->alloc);
        chunk = next;
    }

    static_assert(std::is_trivially_destructible_v<alia_scratch_arena>);
}

void
alia_scratch_reset(alia_scratch_arena* arena)
{
    record_bytes_used(arena);
    update_peak_usage_stats(arena);
    if (arena->first_chunk)
        activate_chunk(arena, arena->first_chunk);
    arena->stats.current.bytes_used = 0;
}

void
alia_scratch_trim_chunks(alia_scratch_arena* arena, uint32_t n)
{
    trim_chunks(arena, n);
}

void*
alia_scratch_alloc(alia_scratch_arena* arena, size_t bytes, size_t align)
{
    return scratch_alloc(arena, bytes, align);
}

alia_scratch_marker
alia_scratch_mark(alia_scratch_arena* arena)
{
    auto* marker = static_cast<alia_scratch_marker_def*>(scratch_alloc(
        arena,
        sizeof(alia_scratch_marker_def),
        alignof(alia_scratch_marker_def)));
    marker->bytes_used = arena->stats.current.bytes_used;
    marker->chunk = arena->active_chunk;
    marker->bump = arena->bump;
    return marker;
}

void
alia_scratch_jump(alia_scratch_arena* arena, alia_scratch_marker m)
{
    record_bytes_used(arena);
    update_peak_usage_stats(arena);
    arena->active_chunk = m->chunk;
    arena->bump = m->bump;
    arena->stats.current.bytes_used = m->bytes_used;
}

alia_scratch_stats
alia_scratch_get_stats(alia_scratch_arena const* arena)
{
    // The stats in the arena should accurate reflect allocations but not
    // usage (which doesn't include bytes allocated from the active chunk).
    alia_scratch_stats s = arena->stats;
    if (arena->active_chunk)
    {
        // Update the current used bytes to include the active chunk.
        s.current.bytes_used
            += arena->bump.head - chunk_start(arena->active_chunk);
        // And update the peak used bytes to reflect that.
        s.peak.bytes_used = std::max(s.peak.bytes_used, s.current.bytes_used);
    }
    return s;
}

} // extern "C"
