#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

LayoutPlacementNode*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space);

} // namespace alia
