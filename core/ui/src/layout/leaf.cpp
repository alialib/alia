#include <alia/ui/layout/leaf.hpp>

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
measure_leaf_horizontal(LayoutScratchArena* scratch, LayoutNode* node)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    return HorizontalRequirements{
        .min_size = leaf.size.x + leaf.margin.x * 2,
        .growth_factor = leaf.base.growth_factor};
}

void
assign_leaf_widths(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
}

VerticalRequirements
measure_leaf_vertical(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    return VerticalRequirements{
        .min_size = leaf.size.y + leaf.margin.y * 2,
        .growth_factor = leaf.base.growth_factor};
}

void
assign_leaf_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    LeafLayoutPlacement* placement
        = reinterpret_cast<LeafLayoutPlacement*>(ctx->arena->allocate(
            sizeof(LeafLayoutPlacement), alignof(LeafLayoutPlacement)));
    placement->position = box.pos + leaf.margin;
    placement->size = leaf.size - leaf.margin * 2;
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
