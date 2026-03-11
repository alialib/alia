#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/style.h>
#include <alia/context.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

namespace alia {

struct growth_override_node
{
    alia_layout_container container;
    float growth;
};

alia_horizontal_requirements
growth_override_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    auto const child_x
        = alia_measure_horizontal(ctx, override.container.first_child);
    return alia_horizontal_requirements{
        .min_size = child_x.min_size, .growth_factor = override.growth};
}

void
growth_override_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    alia_assign_widths(
        ctx, main_axis, override.container.first_child, assigned_width);
}

alia_vertical_requirements
growth_override_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    auto const child_y = alia_measure_vertical(
        ctx, main_axis, override.container.first_child, assigned_width);
    return alia_vertical_requirements{
        .min_size = child_y.min_size,
        .growth_factor = override.growth,
        .ascent = child_y.ascent,
        .descent = child_y.descent};
}

void
growth_override_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    alia_assign_boxes(
        ctx, main_axis, override.container.first_child, box, baseline);
}

alia_horizontal_requirements
growth_override_measure_wrapped_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    auto const child_x
        = alia_measure_wrapped_horizontal(ctx, override.container.first_child);
    return alia_horizontal_requirements{
        .min_size = child_x.min_size, .growth_factor = child_x.growth_factor};
}

alia_wrapping_requirements
growth_override_measure_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    return alia_measure_wrapped_vertical(
        ctx,
        main_axis,
        override.container.first_child,
        current_x_offset,
        line_width);
}

void
growth_override_assign_wrapped_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_wrapping_assignment const* assignment)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    alia_assign_wrapped_boxes(
        ctx, main_axis, override.container.first_child, assignment);
}

alia_layout_node_vtable growth_override_vtable
    = {growth_override_measure_horizontal,
       growth_override_assign_widths,
       growth_override_measure_vertical,
       growth_override_assign_boxes,
       growth_override_measure_wrapped_horizontal,
       growth_override_measure_wrapped_vertical,
       growth_override_assign_wrapped_boxes};

} // namespace alia

using namespace alia;

extern "C" {

struct alia_layout_growth_override_scope
{
    growth_override_node* node;
};

void
alia_layout_growth_override_begin(alia_context* ctx, float growth)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_growth_override_scope>(ctx);
        auto* node
            = arena_alloc<growth_override_node>(ctx->layout->emission.arena);
        *node = growth_override_node{
            .container
            = {.base = {.vtable = &growth_override_vtable, .next_sibling = 0},
               .flags = 0,
               .first_child = 0},
            .growth = growth};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
    }
}

void
alia_layout_growth_override_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_growth_override_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
