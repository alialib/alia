#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

struct LayoutLeafNode
{
    LayoutNode base;
    LayoutProperties props;
    float padding;
    Vec2 size;
};

extern LayoutNodeVtable leaf_vtable;

} // namespace alia
