#pragma once

#include <alia/arena.hpp>
#include <alia/geometry.hpp>
#include <alia/internals/arena.hpp>
#include <alia/layout/container.hpp>

namespace alia {

struct layout_system
{
    alia_arena node_arena;
    alia_arena scratch_arena;
    layout_container root;
    alia_arena placement_arena;
};

void
initialize(layout_system& system);

void
resolve_layout(layout_system& system, alia_vec2f available_space);

} // namespace alia
