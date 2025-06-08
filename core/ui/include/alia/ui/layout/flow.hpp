#pragma once

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_flow_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& flow);

HorizontalRequirements
recall_flow_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& flow);

void
assign_flow_widths(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& flow);

VerticalRequirements
gather_flow_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& flow);

VerticalRequirements
recall_flow_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& flow);

void
assign_flow_boxes(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutSpec const& flow);

} // namespace alia
