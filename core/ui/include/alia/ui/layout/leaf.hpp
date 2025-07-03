#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

struct LayoutLeafNode
{
    LayoutNode base;

    Vec2 size;
    Vec2 margin;
};

extern LayoutNodeVtable leaf_vtable;

} // namespace alia
