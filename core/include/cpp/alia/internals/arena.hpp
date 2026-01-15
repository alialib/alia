#pragma once

#include <alia/arena.h>

#include <alia/base.hpp>

// ABI STRUCTURES

struct alia_arena
{
    alia_bump_state bump{};

    alia_arena_controller controller{};

    // stats - not necessarily accurate between queries
    size_t peak_usage = 0;
};

namespace alia {

inline alia_arena*
arena_from_view(alia_arena_view* view)
{
    static_assert(
        std::is_same<decltype(alia_arena::bump), alia_arena_view>::value,
        "arena_from_view: alia_arena::bump must be of type alia_arena_view");
    static_assert(
        offsetof(alia_arena, bump) == 0,
        "arena_from_view: alia_arena must embed `bump` at offset 0");
    return reinterpret_cast<alia_arena*>(view);
}

} // namespace alia
