#pragma once

#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

struct LayoutLeafNode
{
    LayoutNode base;
    LayoutFlagSet flags;
    float padding;
    Vec2 size;
};

struct LeafLayoutPlacement
{
    Vec2 position;
    Vec2 size;
};

extern LayoutNodeVtable leaf_vtable;

} // namespace alia
