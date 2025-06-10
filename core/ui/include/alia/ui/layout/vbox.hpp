#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_vbox_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& vbox);

HorizontalRequirements
recall_vbox_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& vbox);

void
assign_vbox_widths(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& vbox);

VerticalRequirements
gather_vbox_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& vbox);

VerticalRequirements
recall_vbox_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& vbox);

void
assign_vbox_boxes(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutNode const& vbox);

} // namespace alia
