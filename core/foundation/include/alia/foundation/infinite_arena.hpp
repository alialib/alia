#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <alia/foundation/base.hpp>

namespace alia {

// UniformInfiniteArena is a simple arena allocator that assumes an infinite
// reservation of memory and uniform allocation sizes. (It checks for overflow
// with ALIA_ASSERT but does not check for misalignment.)
struct UniformInfiniteArena
{
    static constexpr std::size_t INFINITE_CAPACITY = 128 * 1024 * 1024;

    bool
    initialize(std::size_t reservation_size = INFINITE_CAPACITY);

    void*
    allocate(std::size_t size)
    {
        ALIA_ASSERT(base_ && "LayoutArena must be initialized");
        void* ptr = next_;
        next_ += size;
        ALIA_ASSERT(next_ - base_ <= capacity_ && "LayoutArena overflow");
        return ptr;
    }

    void
    reset()
    {
        next_ = base_;
    }

    void
    release();

    ~UniformInfiniteArena()
    {
        release();
    }

 private:
    std::uint8_t* base_ = nullptr;
    std::uint8_t* next_ = nullptr;
    std::size_t capacity_ = 0;
};

} // namespace alia
