#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <alia/abi/base/arena.h>

#include <alia/prelude.hpp>

namespace alia {

void
initialize_lazy_commit_arena(
    alia_arena* arena, std::size_t chunk_reservation_size = 128 * 1024 * 1024);

constexpr std::size_t
align_up(std::size_t value)
{
    return (value + (ALIA_MIN_ALIGN - 1)) & ~(ALIA_MIN_ALIGN - 1);
}

template<class T>
T*
arena_alloc(alia_bump_allocator& arena)
{
    static_assert(alignof(T) <= ALIA_MIN_ALIGN, "ALIA_MIN_ALIGN exceeded");
    return reinterpret_cast<T*>(
        alia_arena_ptr(&arena, alia_arena_alloc(&arena, align_up(sizeof(T)))));
}

template<class T>
T*
arena_alloc_trailing(alia_bump_allocator& arena, std::size_t trailing_bytes)
{
    return reinterpret_cast<T*>(alia_arena_ptr(
        &arena,
        alia_arena_alloc(&arena, align_up(sizeof(T) + trailing_bytes))));
}

template<class T>
T*
arena_alloc_array(alia_bump_allocator& arena, std::size_t count)
{
    static_assert(alignof(T) <= ALIA_MIN_ALIGN, "ALIA_MIN_ALIGN exceeded");
    return reinterpret_cast<T*>(alia_arena_ptr(
        &arena, alia_arena_alloc(&arena, align_up(count * sizeof(T)))));
}

template<class T>
T*
arena_new(alia_bump_allocator& arena)
{
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(alignof(T) <= ALIA_MIN_ALIGN, "ALIA_MIN_ALIGN exceeded");
    void* raw = alia_arena_ptr(
        &arena, alia_arena_alloc(&arena, align_up(sizeof(T))));
    return new (raw) T{};
}

} // namespace alia
