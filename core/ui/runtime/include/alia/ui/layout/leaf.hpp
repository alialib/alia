#pragma once

#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

struct layout_leaf_node
{
    layout_node base;
    layout_flag_set flags;
    float padding;
    vec2 size;
};

struct leaf_layout_placement
{
    vec2 position;
    vec2 size;
};

extern layout_node_vtable leaf_vtable;

} // namespace alia
