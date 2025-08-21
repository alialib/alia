#include <alia/ui/layout/leaf.hpp>

#include <alia/ui/layout/utilities.hpp>

namespace alia {

HorizontalRequirements
leaf_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    return HorizontalRequirements{
        .min_size = leaf.size.x + leaf.padding * 2,
        .growth_factor = resolve_growth_factor(leaf.flags)};
}

void
leaf_assign_widths(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
}

VerticalRequirements
leaf_measure_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    return VerticalRequirements{
        .min_size = leaf.size.y + leaf.padding * 2,
        .growth_factor = resolve_growth_factor(leaf.flags),
        .ascent = 0,
        .descent = 0};
}

void
leaf_assign_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    Box box,
    float baseline)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    LeafLayoutPlacement* placement
        = arena_alloc<LeafLayoutPlacement>(*ctx->arena);
    auto const padded_placement = resolve_padded_assignment(
        adjust_flags_for_main_axis(leaf.flags, main_axis),
        box.size,
        baseline,
        leaf.size,
        0,
        leaf.padding);
    placement->position = box.pos + padded_placement.pos;
    placement->size = padded_placement.size;
}

LayoutNodeVtable leaf_vtable
    = {leaf_measure_horizontal,
       leaf_assign_widths,
       leaf_measure_vertical,
       leaf_assign_boxes,
       leaf_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
