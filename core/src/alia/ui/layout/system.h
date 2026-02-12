#pragma once

#include <alia/abi/ui/layout/utilities.h>
#include <alia/base/arena.h>

extern "C" {

struct alia_layout_system
{
    // used to store the nodes of the layout tree
    alia_arena node_arena;
    // used to store intermediate results of layout calculations between passes
    alia_arena scratch_arena;
    // the root node of the layout tree
    alia_layout_container root;
    // used to store the placement information for the nodes in the layout tree
    alia_arena placement_arena;
};

} // extern "C"
