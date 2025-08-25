#include <alia/ui/layout/modifiers/placement_hook.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/system.hpp>

namespace alia {

void
begin_placement_hook(
    Context& ctx, PlacementHookScope& scope, LayoutFlagSet flags)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        PlacementHookNode* node
            = arena_alloc<PlacementHookNode>(*layout.arena);
        *node = PlacementHookNode{
            .base = {.vtable = &placement_hook_vtable, .next_sibling = 0},
            .flags = flags,
            .first_child = 0};
        scope.container.container = node;
        *layout.next_ptr = &node->base;
        layout.next_ptr = &node->first_child;
    }
    else
    {
        scope.placement
            = *arena_alloc<PlacementInfo>(ctx.system->layout.placement_arena);
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
    return measure_horizontal(ctx, placement_hook.first_child);
}

void
placement_hook_assign_widths(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<PlacementHookNode*>(node);
    assign_widths(ctx, main_axis, placement_hook.first_child, assigned_width);
}

VerticalRequirements
placement_hook_measure_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<PlacementHookNode*>(node);
    return measure_vertical(
        ctx, main_axis, placement_hook.first_child, assigned_width);
}

void
placement_hook_assign_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    Box box,
    float baseline)
{
    auto& placement_hook = *reinterpret_cast<PlacementHookNode*>(node);

    PlacementInfo* placement = arena_alloc<PlacementInfo>(*ctx->arena);
    placement->box = box;
    placement->baseline = baseline;

    assign_boxes(ctx, main_axis, placement_hook.first_child, box, baseline);
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
