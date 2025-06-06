#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_vbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& vbox);

HorizontalRequirements
recall_vbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& vbox);

void
assign_vbox_widths(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& vbox);

VerticalRequirements
gather_vbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& vbox);

VerticalRequirements
recall_vbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& vbox);

void
assign_vbox_boxes(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutSpec const& vbox);

} // namespace alia
