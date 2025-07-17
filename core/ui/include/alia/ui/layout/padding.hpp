#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

struct PaddingLayoutNode
{
    LayoutContainer container;
    float padding;
};

extern LayoutNodeVtable padding_vtable;

} // namespace alia
