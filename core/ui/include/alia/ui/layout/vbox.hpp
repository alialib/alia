#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_vbox_x_requirements(LayoutScratchArena& scratch, LayoutNode& vbox);

HorizontalRequirements
recall_vbox_x_requirements(LayoutScratchArena& scratch, LayoutNode& vbox);

void
assign_vbox_widths(
    LayoutScratchArena& scratch, LayoutNode& vbox, float assigned_width);

VerticalRequirements
gather_vbox_y_requirements(
    LayoutScratchArena& scratch, LayoutNode& vbox, float assigned_width);

VerticalRequirements
recall_vbox_y_requirements(LayoutScratchArena& scratch, LayoutNode& vbox);

void
assign_vbox_boxes(PlacementContext& ctx, LayoutNode& vbox, Box box);

} // namespace alia
