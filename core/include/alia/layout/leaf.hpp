#pragma once

#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/protocol.h>

namespace alia {

struct layout_leaf_node
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    float padding;
    alia_vec2f size;
};

struct leaf_layout_placement
{
    alia_vec2f position;
    alia_vec2f size;
};

extern alia_layout_node_vtable leaf_vtable;

} // namespace alia
