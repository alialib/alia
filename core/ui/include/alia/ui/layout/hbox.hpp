#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_hbox_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& hbox);

HorizontalRequirements
recall_hbox_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& hbox);

void
assign_hbox_widths(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& hbox);

VerticalRequirements
gather_hbox_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& hbox);

VerticalRequirements
recall_hbox_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& hbox);

void
assign_hbox_boxes(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutNode const& hbox);

} // namespace alia
