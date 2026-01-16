#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <alia/abi/arena.h>

#include <alia/base.hpp>

namespace alia {

void
initialize_lazy_commit_arena(
    alia_arena* arena, std::size_t chunk_reservation_size = 128 * 1024 * 1024);

constexpr std::size_t
align_up(std::size_t value)
{
    return (value + (ALIA_MIN_ARENA_ALIGN - 1)) & ~(ALIA_MIN_ARENA_ALIGN - 1);
}

template<class T>
T*
arena_alloc(alia_arena_view& arena)
{
    static_assert(
        alignof(T) <= ALIA_MIN_ARENA_ALIGN, "ALIA_MIN_ARENA_ALIGN exceeded");
    return reinterpret_cast<T*>(
        alia_arena_ptr(&arena, alia_arena_alloc(&arena, align_up(sizeof(T)))));
}

template<class T>
T*
arena_array_alloc(alia_arena_view& arena, std::size_t count)
{
    static_assert(
        alignof(T) <= ALIA_MIN_ARENA_ALIGN, "ALIA_MIN_ARENA_ALIGN exceeded");
    return reinterpret_cast<T*>(alia_arena_ptr(
        &arena, alia_arena_alloc(&arena, align_up(count * sizeof(T)))));
}

template<class T>
T*
arena_new(alia_arena_view& arena)
{
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(
        alignof(T) <= ALIA_MIN_ARENA_ALIGN, "ALIA_MIN_ARENA_ALIGN exceeded");
    void* raw = alia_arena_ptr(
        &arena, alia_arena_alloc(&arena, align_up(sizeof(T))));
    return std::construct_at(reinterpret_cast<T*>(raw));
}

} // namespace alia
