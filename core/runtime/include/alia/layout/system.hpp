#pragma once

#include <alia/geometry.hpp>
#include <alia/layout/container.hpp>

namespace alia {

struct layout_system
{
    infinite_arena node_arena;
    infinite_arena scratch_arena;
    layout_container root;
    infinite_arena placement_arena;
};

void
initialize(layout_system& system);

void
resolve_layout(layout_system& system, vec2 available_space);

} // namespace alia
