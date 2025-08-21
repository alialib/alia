#include <alia/kernel/infinite_arena.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace alia {

bool
InfiniteArena::initialize(std::size_t reservation_size)
{
#if defined(_WIN32)
    base_ = (std::uint8_t*) VirtualAlloc(
        nullptr, reservation_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!base_)
        return false;
#else
    base_ = (std::uint8_t*) mmap(
        nullptr,
        reservation_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);
    if (base_ == MAP_FAILED)
    {
        base_ = nullptr;
        return false;
    }
#endif
    capacity_ = reservation_size;
    next_ = base_;
    return true;
}

void
InfiniteArena::release()
{
    if (!base_)
        return;
#if defined(_WIN32)
    VirtualFree(base_, 0, MEM_RELEASE);
#else
    munmap(base_, capacity_);
#endif
    base_ = nullptr;
    capacity_ = 0;
    next_ = nullptr;
}

} // namespace alia
