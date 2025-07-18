#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/resolution.hpp>

namespace alia {

struct InsetLayoutNode
{
    LayoutContainer container;
    Insets insets;
};

extern LayoutNodeVtable inset_vtable;

} // namespace alia
