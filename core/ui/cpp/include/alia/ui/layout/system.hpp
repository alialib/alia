#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/container.hpp>

namespace alia {

struct LayoutSystem
{
    InfiniteArena node_arena;
    InfiniteArena scratch_arena;
    LayoutContainer root;
    InfiniteArena placement_arena;
};

void
initialize(LayoutSystem& system);

void
resolve_layout(LayoutSystem& system, Vec2 available_space);

} // namespace alia
