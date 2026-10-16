#pragma once

#include <alia/abi/ui/layout/utilities/emission.h>
#include <alia/base/arena.h>

#include <stddef.h>
#include <stdint.h>

extern "C" {

// shared placement stream for the main tree and detached regions
struct alia_layout_placement_state
{
    alia_arena arena;
    // generation counter - bumped by `alia_layout_placement_clear`
    uint64_t generation;
    // end offset of the last resolved region in `arena`
    size_t high_water;
};

struct alia_layout_system
{
    // used to store the nodes of the layout tree
    alia_arena node_arena;
    // used to store intermediate results of layout calculations between passes
    alia_arena scratch_arena;
    // the root node of the layout tree
    alia_layout_container root;
    // shared placement stream
    alia_layout_placement_state placement;
};

} // extern "C"
