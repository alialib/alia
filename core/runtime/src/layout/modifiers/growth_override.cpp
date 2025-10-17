#include <alia/layout/modifiers/growth_override.hpp>

#include <alia/context.hpp>

namespace alia {

void
begin_growth_override(
    context& ctx, layout_container_scope& scope, float growth)
{
    if (ctx.pass.type == pass_type::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        growth_override_node* node
            = arena_alloc<growth_override_node>(*layout.arena);
        *node = growth_override_node{
            .container
            = {.base = {.vtable = &growth_override_vtable, .next_sibling = 0},
               .flags = NO_FLAGS,
               .first_child = 0},
            .growth = growth};
        scope.container = &node->container;
        *layout.next_ptr = &node->container.base;
        layout.next_ptr = &node->container.first_child;
    }
}

void
end_growth_override(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

horizontal_requirements
growth_override_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    auto const child_x
        = measure_horizontal(ctx, override.container.first_child);
    return horizontal_requirements{
        .min_size = child_x.min_size, .growth_factor = override.growth};
}

void
growth_override_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    assign_widths(
        ctx, main_axis, override.container.first_child, assigned_width);
}

vertical_requirements
growth_override_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    auto const child_y = measure_vertical(
        ctx, main_axis, override.container.first_child, assigned_width);
    return vertical_requirements{
        .min_size = child_y.min_size,
        .growth_factor = override.growth,
        .ascent = child_y.ascent,
        .descent = child_y.descent};
}

void
growth_override_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    assign_boxes(
        ctx, main_axis, override.container.first_child, box, baseline);
}

horizontal_requirements
growth_override_measure_wrapped_horizontal(
    measurement_context* ctx, layout_node* node)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    auto const child_x
        = measure_wrapped_horizontal(ctx, override.container.first_child);
    return horizontal_requirements{
        .min_size = child_x.min_size, .growth_factor = child_x.growth_factor};
}

wrapping_requirements
growth_override_measure_wrapped_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    return measure_wrapped_vertical(
        ctx,
        main_axis,
        override.container.first_child,
        current_x_offset,
        line_width);
}

void
growth_override_assign_wrapped_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    wrapping_assignment const* assignment)
{
    auto& override = *reinterpret_cast<growth_override_node*>(node);
    assign_wrapped_boxes(
        ctx, main_axis, override.container.first_child, assignment);
}

layout_node_vtable growth_override_vtable
    = {growth_override_measure_horizontal,
       growth_override_assign_widths,
       growth_override_measure_vertical,
       growth_override_assign_boxes,
       growth_override_measure_wrapped_horizontal,
       growth_override_measure_wrapped_vertical,
       growth_override_assign_wrapped_boxes};

} // namespace alia
