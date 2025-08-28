#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <alia/kernel/base.hpp>

namespace alia {

// infinite_arena is a simple arena allocator that assumes an infinite
// reservation of memory.
struct infinite_arena
{
    static constexpr std::size_t INFINITE_CAPACITY = 128 * 1024 * 1024;

    infinite_arena()
    {
    }

    // Disallow copying.
    infinite_arena(infinite_arena const&) = delete;
    infinite_arena&
    operator=(infinite_arena const&)
        = delete;

    infinite_arena(infinite_arena&& other)
    {
        base_ = other.base_;
        next_ = other.next_;
        capacity_ = other.capacity_;
        // The moved-from arena must have a noop destruction.
        other.base_ = nullptr;
    }
    infinite_arena&
    operator=(infinite_arena&& other)
    {
        base_ = other.base_;
        next_ = other.next_;
        capacity_ = other.capacity_;
        // The moved-from arena must have a noop destruction.
        other.base_ = nullptr;
    }

    ~infinite_arena()
    {
        release();
    }

    bool
    initialize(std::size_t reservation_size = INFINITE_CAPACITY);

    void
    align(std::size_t alignment)
    {
        next_ = (std::uint8_t*) ((std::uintptr_t(next_) + (alignment - 1))
                                 & ~(alignment - 1));
    }

    void*
    alloc(std::size_t size)
    {
        ALIA_ASSERT(base_ && "infinite_arena must be initialized");
        next_ += size;
        ALIA_ASSERT(next_ - base_ <= capacity_ && "infinite_arena overflow");
        return (void*) next_;
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

    std::uint8_t*
    save_state()
    {
        return next_;
    }

    void
    restore_state(std::uint8_t* state)
    {
        next_ = state;
    }

    void
    release();

 private:
    std::uint8_t* base_ = nullptr;
    std::uint8_t* next_ = nullptr;
    std::size_t capacity_ = 0;
};

inline void*
aligned_alloc(infinite_arena& arena, std::size_t size, std::size_t alignment)
{
    std::uintptr_t addr = std::uintptr_t(arena.peek());
    std::uintptr_t aligned_addr = (addr + (alignment - 1)) & ~(alignment - 1);
    std::size_t padding = aligned_addr - addr;
    arena.alloc(size + padding);
    return (void*) aligned_addr;
}

template<class T>
T*
arena_alloc(infinite_arena& arena)
{
    return reinterpret_cast<T*>(aligned_alloc(arena, sizeof(T), alignof(T)));
}

template<class T>
T*
arena_array_alloc(infinite_arena& arena, std::size_t count)
{
    return reinterpret_cast<T*>(
        aligned_alloc(arena, count * sizeof(T), alignof(T)));
}

template<class T>
T*
arena_new(infinite_arena& arena)
{
    static_assert(std::is_trivially_destructible_v<T>);
    void* raw = aligned_alloc(arena, sizeof(T), alignof(T));
    return std::construct_at(reinterpret_cast<T*>(raw));
}

} // namespace alia
