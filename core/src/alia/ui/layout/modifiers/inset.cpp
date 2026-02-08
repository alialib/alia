#include <alia/layout/modifiers/inset.hpp>

#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/style.h>
#include <alia/arena.hpp>
#include <alia/context.hpp>
#include <alia/events.hpp>

using namespace alia::operators;

namespace alia {

void
begin_inset(
    context& ctx,
    layout_container_scope& scope,
    alia_insets insets,
    alia_layout_flags_t flags)
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

alia_horizontal_requirements
inset_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_x
        = alia_measure_horizontal(ctx, inset.container.first_child);
    return alia_horizontal_requirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

void
inset_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    alia_assign_widths(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.left - inset.insets.right);
}

alia_vertical_requirements
inset_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_y = alia_measure_vertical(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.top - inset.insets.bottom);
    return alia_vertical_requirements{
        .min_size = child_y.min_size + inset.insets.top + inset.insets.bottom,
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent + inset.insets.top,
        .descent = child_y.descent + inset.insets.bottom};
}

void
inset_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
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
    alia_assign_boxes(
        ctx,
        main_axis,
        inset.container.first_child,
        child_box,
        baseline - inset.insets.top);
}

alia_horizontal_requirements
inset_measure_wrapped_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_x
        = alia_measure_wrapped_horizontal(ctx, inset.container.first_child);
    return alia_horizontal_requirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

alia_wrapping_requirements
inset_measure_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);

    auto const child = alia_measure_wrapped_vertical(
        ctx,
        main_axis,
        inset.container.first_child,
        current_x_offset,
        line_width - inset.insets.left - inset.insets.right);

    alia_wrapping_requirements requirements = child;

    if (alia_layout_wrapping_has_first_line_content(child))
    {
        requirements.first_line.height += inset.insets.top;
        requirements.first_line.ascent += inset.insets.top;
    }
    else if (alia_layout_wrapping_has_wrapped_content(child))
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

    if (alia_layout_wrapping_has_wrapped_content(child))
    {
        requirements.last_line.height += inset.insets.bottom;
        requirements.last_line.descent += inset.insets.bottom;
    }
    else if (alia_layout_wrapping_has_first_line_content(child))
    {
        requirements.first_line.height += inset.insets.bottom;
        requirements.first_line.descent += inset.insets.bottom;
    }

    requirements.end_x += inset.insets.left + inset.insets.right;

    return requirements;
}

void
inset_assign_wrapped_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_wrapping_assignment const* assignment)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    alia_wrapping_assignment inset_assignment = *assignment;
    inset_assignment.x_base += inset.insets.left;
    inset_assignment.line_width -= inset.insets.left + inset.insets.right;
    alia_assign_wrapped_boxes(
        ctx, main_axis, inset.container.first_child, &inset_assignment);
}

alia_layout_node_vtable inset_vtable
    = {inset_measure_horizontal,
       inset_assign_widths,
       inset_measure_vertical,
       inset_assign_boxes,
       inset_measure_wrapped_horizontal,
       inset_measure_wrapped_vertical,
       inset_assign_wrapped_boxes};

} // namespace alia
