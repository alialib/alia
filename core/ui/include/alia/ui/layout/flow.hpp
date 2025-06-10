#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_flow_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& flow);

HorizontalRequirements
recall_flow_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& flow);

void
assign_flow_widths(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& flow);

VerticalRequirements
gather_flow_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& flow);

VerticalRequirements
recall_flow_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& flow);

void
assign_flow_boxes(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutNode const& flow);

} // namespace alia
