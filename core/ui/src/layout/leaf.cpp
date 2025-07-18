#include <alia/ui/layout/leaf.hpp>

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
measure_leaf_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    return HorizontalRequirements{
        .min_size = leaf.size.x + leaf.padding * 2,
        .growth_factor = float(leaf.props.growth_factor)};
}

void
assign_leaf_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
}

VerticalRequirements
measure_leaf_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    return VerticalRequirements{
        .min_size = leaf.size.y + leaf.padding * 2,
        .growth_factor = float(leaf.props.growth_factor),
        .ascent = 0,
        .descent = 0};
}

void
assign_leaf_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    LeafLayoutPlacement* placement
        = reinterpret_cast<LeafLayoutPlacement*>(ctx->arena->allocate(
            sizeof(LeafLayoutPlacement), alignof(LeafLayoutPlacement)));
    auto const padded_placement = resolve_padded_assignment(
        leaf.props, box.size, baseline, leaf.size, 0, leaf.padding);
    placement->position = box.pos + padded_placement.pos;
    placement->size = padded_placement.size;
    *ctx->next_ptr = &placement->base;
    ctx->next_ptr = &placement->base.next;
}

LayoutNodeVtable leaf_vtable
    = {measure_leaf_horizontal,
       assign_leaf_widths,
       measure_leaf_vertical,
       assign_leaf_boxes,
       default_measure_wrapped_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
