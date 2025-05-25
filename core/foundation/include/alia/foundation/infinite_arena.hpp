#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <alia/foundation/base.hpp>

namespace alia {

// RawInfiniteArena is a simple arena allocator that assumes an infinite
// reservation of memory. It's up to the caller to ensure that allocations:
//
// - Do not overflow the arena.
// - Are properly aligned.
//
// It checks for overflow with ALIA_ASSERT, but does not check for alignment.
//
struct RawInfiniteArena
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

    void*
    peek()
    {
        return next_;
    }

    void
    reset()
    {
        next_ = base_;
    }

    void
    release();

    ~RawInfiniteArena()
    {
        release();
    }

 private:
    std::uint8_t* base_ = nullptr;
    std::uint8_t* next_ = nullptr;
    std::size_t capacity_ = 0;
};

// UniformlyAlignedInfiniteArena is a simple arena allocator that assumes an
// infinite reservation of memory and uniform alignment requirements. (It
// checks for overflow with ALIA_ASSERT but does not check for misalignment.)
template<std::size_t Alignment>
struct UniformlyAlignedInfiniteArena
{
    // TODO: Use inheritance to avoid code duplication?

    static constexpr std::size_t INFINITE_CAPACITY = 128 * 1024 * 1024;

    RawInfiniteArena base_;

    bool
    initialize(std::size_t reservation_size = INFINITE_CAPACITY)
    {
        return base_.initialize(reservation_size);
    }

    void*
    allocate(std::size_t size, std::size_t alignment)
    {
        ALIA_ASSERT(alignment <= Alignment);
        std::size_t const aligned_size
            = (size + Alignment - 1) & ~(Alignment - 1);
        return base_.allocate(aligned_size);
    }

    void*
    peek()
    {
        return base_.peek();
    }

    void
    reset()
    {
        base_.reset();
    }

    void
    release()
    {
        base_.release();
    }
};

} // namespace alia
