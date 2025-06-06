#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_hbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& hbox);

HorizontalRequirements
recall_hbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& hbox);

void
assign_hbox_widths(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& hbox);

VerticalRequirements
gather_hbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& hbox);

VerticalRequirements
recall_hbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& hbox);

void
assign_hbox_boxes(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutSpec const& hbox);

} // namespace alia
