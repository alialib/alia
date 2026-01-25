#include <alia/layout/modifiers/min_size.hpp>

#include <alia/abi/ui/style.h>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/layout/utilities.hpp>

namespace alia {

void
begin_min_size_constraint(
    context& ctx, layout_container_scope& scope, vec2f min_size)
{
    if (is_refresh_event(ctx))
    {
        auto& layout = as_refresh_event(ctx).layout_emission;
        min_size_node* node = arena_alloc<min_size_node>(*layout.arena);
        *node = min_size_node{
            .container
            = {.base = {.vtable = &min_size_vtable, .next_sibling = 0},
               .flags = NO_FLAGS,
               .first_child = 0},
            .min_size = min_size};
        scope.container = &node->container;
        *layout.next_ptr = &node->container.base;
        layout.next_ptr = &node->container.first_child;
    }
}

void
end_min_size_constraint(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

horizontal_requirements
min_size_measure_horizontal(measurement_context* ctx, layout_node* base_node)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    auto const child_x = measure_horizontal(ctx, node.container.first_child);
    return horizontal_requirements{
        .min_size = std::max(node.min_size.x, child_x.min_size),
        .growth_factor = child_x.growth_factor};
}

void
min_size_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    assign_widths(ctx, main_axis, node.container.first_child, assigned_width);
}

vertical_requirements
min_size_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    auto const child_y = measure_vertical(
        ctx, main_axis, node.container.first_child, assigned_width);
    return vertical_requirements{
        .min_size = std::max(node.min_size.y, child_y.min_size),
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent,
        .descent = child_y.descent};
}

void
min_size_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* base_node,
    box box,
    float baseline)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    assign_boxes(ctx, main_axis, node.container.first_child, box, baseline);
}

layout_node_vtable min_size_vtable
    = {min_size_measure_horizontal,
       min_size_assign_widths,
       min_size_measure_vertical,
       min_size_assign_boxes,
       min_size_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
