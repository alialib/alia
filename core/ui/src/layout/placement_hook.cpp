#include <alia/ui/layout/placement_hook.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/scratch.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

struct PlacementHookPlacement
{
    LayoutPlacementNode base;
    PlacementInfo placement;
};

void
begin_placement_hook(
    Context& ctx, PlacementHookScope& scope, LayoutFlagSet flags)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        PlacementHookNode* this_node
            = arena_alloc<PlacementHookNode>(*layout.arena);
        scope.container.this_container = this_node;
        *this_node = PlacementHookNode{
            .base = {.vtable = &placement_hook_vtable, .next_sibling = 0},
            .flags = flags,
            .child_count = 0,
            .first_child = 0};
        *layout.next_ptr = &this_node->base;
        layout.next_ptr = &this_node->first_child;
        scope.container.parent_container = layout.active_container;
        ++scope.container.parent_container->child_count;
        layout.active_container = scope.container.this_container;
    }
    else
    {
        auto const* placement = ctx.layout_consumption.next_placement;
        ctx.layout_consumption.next_placement = placement->next;
        auto& placement_info = *downcast<PlacementHookPlacement>(placement);
        scope.placement = placement_info.placement;
    }
}

void
end_placement_hook(Context& ctx, PlacementHookScope& scope)
{
    end_container(ctx, scope.container);
}

HorizontalRequirements
placement_hook_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& placement_hook = *reinterpret_cast<PlacementHookNode*>(node);
    ALIA_ASSERT(placement_hook.child_count == 1);
    return measure_horizontal(ctx, placement_hook.first_child);
}

void
placement_hook_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<PlacementHookNode*>(node);
    ALIA_ASSERT(placement_hook.child_count == 1);
    assign_widths(ctx, placement_hook.first_child, assigned_width);
}

VerticalRequirements
placement_hook_measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<PlacementHookNode*>(node);
    ALIA_ASSERT(placement_hook.child_count == 1);
    return measure_vertical(ctx, placement_hook.first_child, assigned_width);
}

void
placement_hook_assign_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& placement_hook = *reinterpret_cast<PlacementHookNode*>(node);

    PlacementHookPlacement* placement
        = arena_alloc<PlacementHookPlacement>(*ctx->arena);
    placement->placement.box = box;
    placement->placement.baseline = baseline;
    *ctx->next_ptr = &placement->base;
    ctx->next_ptr = &placement->base.next;

    ALIA_ASSERT(placement_hook.child_count == 1);
    assign_boxes(ctx, placement_hook.first_child, box, baseline);
}

LayoutNodeVtable placement_hook_vtable
    = {placement_hook_measure_horizontal,
       placement_hook_assign_widths,
       placement_hook_measure_vertical,
       placement_hook_assign_boxes,
       placement_hook_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
