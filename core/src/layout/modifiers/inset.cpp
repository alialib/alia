#include <alia/layout/modifiers/inset.hpp>

#include <alia/abi/ui/style.h>
#include <alia/context.hpp>
#include <alia/events.hpp>

using namespace alia::operators;

namespace alia {

void
begin_inset(
    context& ctx,
    layout_container_scope& scope,
    alia_insets insets,
    layout_flag_set flags)
{
    if (is_refresh_event(ctx))
    {
        auto& layout = as_refresh_event(ctx).layout_emission;
        inset_layout_node* node
            = arena_alloc<inset_layout_node>(*layout.arena);
        *node = inset_layout_node{
            .container
            = {.base = {.vtable = &inset_vtable, .next_sibling = 0},
               .flags = flags,
               .first_child = 0},
            .insets = insets};
        scope.container = &node->container;
        *layout.next_ptr = &node->container.base;
        layout.next_ptr = &node->container.first_child;
    }
}

void
end_inset(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

horizontal_requirements
inset_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_x = measure_horizontal(ctx, inset.container.first_child);
    return horizontal_requirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

void
inset_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    assign_widths(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.left - inset.insets.right);
}

vertical_requirements
inset_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_y = measure_vertical(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.top - inset.insets.bottom);
    return vertical_requirements{
        .min_size = child_y.min_size + inset.insets.top + inset.insets.bottom,
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent + inset.insets.top,
        .descent = child_y.descent + inset.insets.bottom};
}

void
inset_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    alia_box box,
    float baseline)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_box = alia_box{
        .min = box.min + alia_vec2f{inset.insets.left, inset.insets.top},
        .size = box.size
              - alia_vec2f{
                  inset.insets.left + inset.insets.right,
                  inset.insets.top + inset.insets.bottom}};
    assign_boxes(
        ctx,
        main_axis,
        inset.container.first_child,
        child_box,
        baseline - inset.insets.top);
}

horizontal_requirements
inset_measure_wrapped_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_x
        = measure_wrapped_horizontal(ctx, inset.container.first_child);
    return horizontal_requirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

wrapping_requirements
inset_measure_wrapped_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);

    auto const child = measure_wrapped_vertical(
        ctx,
        main_axis,
        inset.container.first_child,
        current_x_offset,
        line_width - inset.insets.left - inset.insets.right);

    wrapping_requirements requirements = child;

    if (has_first_line_content(child))
    {
        requirements.first_line.height += inset.insets.top;
        requirements.first_line.ascent += inset.insets.top;
    }
    else if (has_wrapped_content(child))
    {
        if (requirements.interior_height > 0)
        {
            requirements.interior_height += inset.insets.top;
        }
        else
        {
            requirements.last_line.height += inset.insets.top;
            requirements.last_line.ascent += inset.insets.top;
        }
    }

    if (has_wrapped_content(child))
    {
        requirements.last_line.height += inset.insets.bottom;
        requirements.last_line.descent += inset.insets.bottom;
    }
    else if (has_first_line_content(child))
    {
        requirements.first_line.height += inset.insets.bottom;
        requirements.first_line.descent += inset.insets.bottom;
    }

    requirements.end_x += inset.insets.left + inset.insets.right;

    return requirements;
}

void
inset_assign_wrapped_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    wrapping_assignment const* assignment)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    wrapping_assignment inset_assignment = *assignment;
    inset_assignment.x_base += inset.insets.left;
    inset_assignment.line_width -= inset.insets.left + inset.insets.right;
    assign_wrapped_boxes(
        ctx, main_axis, inset.container.first_child, &inset_assignment);
}

layout_node_vtable inset_vtable
    = {inset_measure_horizontal,
       inset_assign_widths,
       inset_measure_vertical,
       inset_assign_boxes,
       inset_measure_wrapped_horizontal,
       inset_measure_wrapped_vertical,
       inset_assign_wrapped_boxes};

} // namespace alia
