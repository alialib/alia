#include <alia/layout/modifiers/placement_hook.hpp>

#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/layout/utilities.hpp>
#include <alia/system/object.hpp>

namespace alia {

void
begin_placement_hook(
    context& ctx, placement_hook_scope& scope, layout_flag_set flags)
{
    if (is_refresh_event(ctx))
    {
        auto& layout = as_refresh_event(ctx).layout_emission;
        placement_hook_node* node
            = arena_alloc<placement_hook_node>(*layout.arena);
        *node = placement_hook_node{
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
            = *arena_alloc<placement_info>(ctx.system->layout.placement_arena);
    }
}

void
end_placement_hook(context& ctx, placement_hook_scope& scope)
{
    end_container(ctx, scope.container);
}

horizontal_requirements
placement_hook_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);
    return measure_horizontal(ctx, placement_hook.first_child);
}

void
placement_hook_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);
    assign_widths(ctx, main_axis, placement_hook.first_child, assigned_width);
}

vertical_requirements
placement_hook_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);
    return measure_vertical(
        ctx, main_axis, placement_hook.first_child, assigned_width);
}

void
placement_hook_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);

    placement_info* placement = arena_alloc<placement_info>(*ctx->arena);
    placement->box = box;
    placement->baseline = baseline;

    assign_boxes(ctx, main_axis, placement_hook.first_child, box, baseline);
}

layout_node_vtable placement_hook_vtable
    = {placement_hook_measure_horizontal,
       placement_hook_assign_widths,
       placement_hook_measure_vertical,
       placement_hook_assign_boxes,
       placement_hook_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
