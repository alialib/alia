#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

struct LayoutLeafNode
{
    LayoutNode base;
    LayoutFlagSet flags;
    float padding;
    Vec2 size;
};

extern LayoutNodeVtable leaf_vtable;

} // namespace alia
