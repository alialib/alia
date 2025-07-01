#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_hbox_x_requirements(LayoutScratchArena& scratch, LayoutNode& hbox);

HorizontalRequirements
recall_hbox_x_requirements(LayoutScratchArena& scratch, LayoutNode& hbox);

void
assign_hbox_widths(
    LayoutScratchArena& scratch, LayoutNode& hbox, float assigned_width);

VerticalRequirements
gather_hbox_y_requirements(
    LayoutScratchArena& scratch, LayoutNode& hbox, float assigned_width);

VerticalRequirements
recall_hbox_y_requirements(LayoutScratchArena& scratch, LayoutNode& hbox);

void
assign_hbox_boxes(PlacementContext& ctx, LayoutNode& hbox, Box box);

} // namespace alia
