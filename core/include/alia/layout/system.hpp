#pragma once

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/base/arena.h>
#include <alia/layout/container.hpp>

namespace alia {

struct layout_system
{
    // used to store the nodes of the layout tree
    alia_arena node_arena;
    // used to store intermediate results of layout calculations between passes
    alia_arena scratch_arena;
    // the root node of the layout tree
    layout_container root;
    // used to store the placement information for the nodes in the layout tree
    alia_arena placement_arena;
};

void
initialize(layout_system& system);

void
resolve_layout(layout_system& system, alia_vec2f available_space);

} // namespace alia
