#include <alia/abi/base/arena.h>
#include <alia/abi/ui/layout/components.h>
#include <alia/abi/ui/style.h>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/ui/layout.hpp>
#include <alia/system/object.hpp>

namespace alia {

using placement_hook_node = alia_layout_container;

alia_horizontal_requirements
placement_hook_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);
    return alia_measure_horizontal(ctx, placement_hook.first_child);
}

void
placement_hook_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);
    alia_assign_widths(
        ctx, main_axis, placement_hook.first_child, assigned_width);
}

alia_vertical_requirements
placement_hook_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);
    return alia_measure_vertical(
        ctx, main_axis, placement_hook.first_child, assigned_width);
}

void
placement_hook_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& placement_hook = *reinterpret_cast<placement_hook_node*>(node);

    alia_layout_placement* placement
        = arena_alloc<alia_layout_placement>(ctx->arena);
    placement->box = box;
    placement->baseline = baseline;

    alia_assign_boxes(
        ctx, main_axis, placement_hook.first_child, box, baseline);
}

alia_layout_node_vtable placement_hook_vtable
    = {placement_hook_measure_horizontal,
       placement_hook_assign_widths,
       placement_hook_measure_vertical,
       placement_hook_assign_boxes,
       placement_hook_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

} // namespace alia

using namespace alia;

extern "C" {

struct alia_layout_placement_hook_scope
{
    placement_hook_node* node;
    alia_layout_placement placement;
};

alia_layout_placement*
alia_layout_placement_hook_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    auto& scope = stack_push<alia_layout_placement_hook_scope>(ctx);
    if (is_refresh_event(*ctx))
    {
        auto* node
            = arena_alloc<placement_hook_node>(ctx->layout->emission.arena);
        *node = placement_hook_node{
            .base = {.vtable = &placement_hook_vtable, .next_sibling = 0},
            .flags = flags,
            .first_child = 0};
        scope.node = node;
        alia_layout_container_activate(ctx, node);
    }
    else
    {
        scope.placement = *arena_alloc<alia_layout_placement>(
            *alia_layout_placement_arena(ctx));
    }
    return &scope.placement;
}

void
alia_layout_placement_hook_end(alia_context* ctx)
{
    auto& scope = stack_pop<alia_layout_placement_hook_scope>(ctx);
    if (is_refresh_event(*ctx))
    {
        alia_layout_container_deactivate(ctx, scope.node);
    }
}

} // extern "C"
