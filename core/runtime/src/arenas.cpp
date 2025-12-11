#include <alia/arenas.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace alia {

alia_scratch_chunk_allocation
allocate_virtual_chunk(
    void* user, size_t existing_capacity, size_t minimum_needed_bytes)
{
    auto* allocator = static_cast<lazy_commit_arena_allocator*>(user);
#if defined(_WIN32)
    // TODO: handle failure
    return {
        (std::uint8_t*) VirtualAlloc(
            nullptr,
            allocator->chunk_reservation_size,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE),
        allocator->chunk_reservation_size};
#else
    // TODO: handle failure
    return {
        (std::uint8_t*) mmap(
            nullptr,
            allocator->chunk_reservation_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0),
        allocator->chunk_reservation_size};
#endif
}

void
release_virtual_chunk(void* user, alia_scratch_chunk_allocation chunk)
{
    auto* allocator = static_cast<lazy_commit_arena_allocator*>(user);
#if defined(_WIN32)
    VirtualFree(chunk.ptr, 0, MEM_RELEASE);
#else
    munmap(chunk.ptr, chunk.size);
#endif
}

alia_scratch_allocator
make_lazy_commit_arena_allocator(
    lazy_commit_arena_allocator* storage, std::size_t chunk_reservation_size)
{
    storage->chunk_reservation_size = chunk_reservation_size;
    return {
        .user = storage,
        .alloc = allocate_virtual_chunk,
        .free = release_virtual_chunk,
    };
}

} // namespace alia
