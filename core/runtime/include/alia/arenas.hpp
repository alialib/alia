#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <alia/scratch.h>

#include <alia/base.hpp>

namespace alia {

struct lazy_commit_arena_allocator
{
    std::size_t chunk_reservation_size;
};

alia_scratch_allocator
make_lazy_commit_arena_allocator(
    lazy_commit_arena_allocator* storage,
    std::size_t chunk_reservation_size = 128 * 1024 * 1024);

template<class T>
T*
arena_alloc(alia_scratch_arena& arena)
{
    return reinterpret_cast<T*>(
        alia_scratch_alloc(&arena, sizeof(T), alignof(T)));
}

template<class T>
T*
arena_array_alloc(alia_scratch_arena& arena, std::size_t count)
{
    return reinterpret_cast<T*>(
        alia_scratch_alloc(&arena, count * sizeof(T), alignof(T)));
}

template<class T>
T*
arena_new(alia_scratch_arena& arena)
{
    static_assert(std::is_trivially_destructible_v<T>);
    void* raw = alia_scratch_alloc(&arena, sizeof(T), alignof(T));
    return std::construct_at(reinterpret_cast<T*>(raw));
}

} // namespace alia
