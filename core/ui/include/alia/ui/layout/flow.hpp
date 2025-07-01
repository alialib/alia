#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_flow_x_requirements(LayoutScratchArena& scratch, LayoutNode& flow);

HorizontalRequirements
recall_flow_x_requirements(LayoutScratchArena& scratch, LayoutNode& flow);

void
assign_flow_widths(
    LayoutScratchArena& scratch, LayoutNode& flow, float assigned_width);

VerticalRequirements
gather_flow_y_requirements(
    LayoutScratchArena& scratch, LayoutNode& flow, float assigned_width);

VerticalRequirements
recall_flow_y_requirements(LayoutScratchArena& scratch, LayoutNode& flow);

void
assign_flow_boxes(PlacementContext& ctx, LayoutNode& flow, Box box);

} // namespace alia
