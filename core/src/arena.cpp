#include <alia/arena.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <alia/internals/arena.hpp>
#include <alia/prelude.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// ABI FUNCTIONS

using namespace alia;

extern "C" {

alia_struct_spec
alia_arena_object_spec(void)
{
    return {sizeof(alia_arena), alignof(alia_arena)};
}

alia_arena*
alia_arena_init(
    void* mem, void* base, size_t capacity, alia_arena_controller controller)
{
    ALIA_ASSERT(
        (reinterpret_cast<uintptr_t>(base) & (ALIA_MAX_ALIGN - 1)) == 0);
    return new (mem) alia_arena{
        .bump = {.base = static_cast<uint8_t*>(base), .capacity = capacity},
        .controller = controller,
        .peak_usage = 0};
}

void
alia_arena_cleanup(alia_arena* arena)
{
    if (arena->controller.free)
    {
        arena->controller.free(
            arena->controller.user, arena->bump.base, arena->bump.capacity);
    }
    static_assert(std::is_trivially_destructible_v<alia_arena>);
}

alia_arena_view*
alia_arena_get_view(alia_arena* arena)
{
    return &arena->bump;
}

void
alia_update_peak_usage(alia_arena_view* view)
{
    alia_arena* arena = arena_from_view(view);
    arena->peak_usage = (std::max) (arena->peak_usage, arena->bump.offset);
}

void
alia_arena_handle_out_of_memory(alia_arena_view* view, size_t bytes_requested)
{
    alia_arena* arena = arena_from_view(view);
    arena->bump.capacity = arena->controller.grow(
        arena->controller.user,
        arena->bump.base,
        arena->bump.capacity,
        bytes_requested);
}

alia_offset
alia_arena_alloc_aligned(alia_arena_view* arena, size_t bytes, size_t align)
{
    ALIA_ASSERT((bytes & (align - 1)) == 0);
    ALIA_ASSERT(align <= ALIA_MAX_ALIGN);

    if (align <= ALIA_MIN_ALIGN)
        return alia_arena_alloc(arena, bytes);

    size_t aligned_offset = align_offset(arena->offset, align);
    if (aligned_offset + bytes > arena->capacity)
        alia_arena_handle_out_of_memory(arena, bytes);

    arena->offset = aligned_offset + bytes;
    return aligned_offset;
}

alia_arena_stats
alia_arena_get_stats(alia_arena* arena)
{
    alia_update_peak_usage(alia_arena_get_view(arena));
    return {
        .current_usage = arena->bump.offset, .peak_usage = arena->peak_usage};
}

} // extern "C"

// LAZY COMMIT ARENA

namespace alia {

void*
allocate_virtual_block(std::size_t size)
{
#if defined(_WIN32)
    // TODO: handle failure
    return VirtualAlloc(
        nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    // TODO: handle failure
    return mmap(
        nullptr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);
#endif
}

void
release_virtual_block(void* block, std::size_t size)
{
#if defined(_WIN32)
    VirtualFree(block, 0, MEM_RELEASE);
#else
    munmap(block, size);
#endif
}

void
lazy_commit_arena_free_fn(void*, void* base, std::size_t capacity)
{
    release_virtual_block(base, capacity);
}
void
initialize_lazy_commit_arena(
    alia_arena* arena, std::size_t chunk_reservation_size)
{
    void* block = allocate_virtual_block(chunk_reservation_size);
    alia_arena_init(
        arena,
        block,
        chunk_reservation_size,
        alia_arena_controller{
            .user = nullptr,
            .grow = nullptr,
            .free = lazy_commit_arena_free_fn});
}

} // namespace alia
