#include <alia/ui/layout/leaf.hpp>

#include <alia/ui/layout/resolution.hpp>

namespace alia {

HorizontalRequirements
gather_leaf_x_requirements(LayoutScratchArena* scratch, LayoutNode* node)
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
gather_leaf_y_requirements(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    return VerticalRequirements{
        .min_size = leaf.size.y + leaf.margin.y * 2,
        .growth_factor = leaf.base.growth_factor};
}

void
assign_leaf_boxes(PlacementContext* ctx, LayoutNode* node, Box box)
{
    auto& leaf = *reinterpret_cast<LayoutLeafNode*>(node);
    LayoutPlacement* placement
        = reinterpret_cast<LayoutPlacement*>(ctx->arena->allocate(
            sizeof(LayoutPlacement), alignof(LayoutPlacement)));
    placement->position = box.pos + leaf.margin;
    placement->size = leaf.size - leaf.margin * 2;
    *ctx->next_ptr = placement;
    ctx->next_ptr = &placement->next;
}

LayoutNodeVtable leaf_vtable = {
    gather_leaf_x_requirements,
    assign_leaf_widths,
    gather_leaf_y_requirements,
    assign_leaf_boxes,
};

} // namespace alia
