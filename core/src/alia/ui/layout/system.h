#pragma once

#include <alia/abi/base/memory.h>
#include <alia/abi/ui/layout/utilities/emission.h>

#include <stddef.h>
#include <stdint.h>

extern "C" {

struct alia_layout_placement_state
{
    // generation counter - bumped by `alia_layout_placement_clear`
    uint64_t generation;
    // end offset of the last resolved region
    size_t high_water;
};

struct alia_layout_system
{
    // pool table that owns the layout backing
    alia_memory_system* memory;
    // pool ID for the layout tree nodes
    alia_memory_pool_id node_pool;
    // pool ID for measurement scratch
    alia_memory_pool_id scratch_pool;
    // pool ID for the placement stream
    alia_memory_pool_id placement_pool;
    // the root node of the layout tree
    alia_layout_container root;
    // shared placement stream
    alia_layout_placement_state placement;
};

} // extern "C"
