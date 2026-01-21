#pragma once

#include <alia/layout/flags.hpp>
#include <alia/layout/node.hpp>

namespace alia {

struct layout_leaf_node
{
    layout_node base;
    layout_flag_set flags;
    float padding;
    alia_vec2f size;
};

struct leaf_layout_placement
{
    alia_vec2f position;
    alia_vec2f size;
};

extern layout_node_vtable leaf_vtable;

} // namespace alia
