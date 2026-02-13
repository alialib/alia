#include <alia/base/arena.h>
#include <alia/prelude.hpp>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>

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
        .base = static_cast<uint8_t*>(base),
        .capacity = capacity,
        .controller = controller,
        .peak_usage = 0};
}

void
alia_arena_cleanup(alia_arena* arena)
{
    if (arena->controller.free)
    {
        arena->controller.free(
            arena->controller.user, arena->base, arena->capacity);
    }
    static_assert(std::is_trivially_destructible_v<alia_arena>);
}

void
alia_bump_allocator_init(alia_bump_allocator* alloc, alia_arena* arena)
{
    alloc->arena = arena;
    alloc->base = arena->base;
    alloc->capacity = arena->capacity;
    alloc->offset = 0;
    alloc->peak = 0;
}

void
alia_bump_allocator_commit_peak(alia_bump_allocator* alloc)
{
    if (alloc->offset > alloc->peak)
        alloc->peak = alloc->offset;
    if (alloc->arena && alloc->peak > alloc->arena->peak_usage)
        alloc->arena->peak_usage = alloc->peak;
}

void
alia_update_peak_usage(alia_bump_allocator* alloc)
{
    if (alloc->offset > alloc->peak)
        alloc->peak = alloc->offset;
}

void
alia_arena_handle_out_of_memory(
    alia_bump_allocator* alloc, size_t bytes_requested)
{
    alia_arena* arena = alloc->arena;
    ALIA_ASSERT(arena && arena->controller.grow);
    arena->capacity = arena->controller.grow(
        arena->controller.user,
        arena->base,
        arena->capacity,
        bytes_requested);
    alloc->base = arena->base;
    alloc->capacity = arena->capacity;
}

alia_offset
alia_arena_alloc_aligned(
    alia_bump_allocator* alloc, size_t bytes, size_t align)
{
    ALIA_ASSERT((bytes & (align - 1)) == 0);
    ALIA_ASSERT(align <= ALIA_MAX_ALIGN);

    if (align <= ALIA_MIN_ALIGN)
        return alia_arena_alloc(alloc, bytes);

    size_t aligned_offset = align_offset(alloc->offset, align);
    if (aligned_offset + bytes > alloc->capacity)
        alia_arena_handle_out_of_memory(alloc, bytes);

    alloc->offset = aligned_offset + bytes;
    return aligned_offset;
}

alia_arena_stats
alia_arena_get_stats(alia_arena* arena)
{
    return {
        .current_usage = 0,
        .peak_usage = arena->peak_usage,
    };
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
