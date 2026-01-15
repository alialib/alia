#pragma once

#include <alia/arena.hpp>
#include <alia/geometry.hpp>
#include <alia/layout.h>

namespace alia {

using line_requirements = alia_line_requirements;
using wrapping_requirements = alia_wrapping_requirements;
using horizontal_requirements = alia_horizontal_requirements;
using vertical_requirements = alia_vertical_requirements;
using measurement_context = alia_measurement_context;
using placement_context = alia_placement_context;
using layout_node = alia_layout_node;
using wrapping_assignment = alia_wrapping_assignment;
using vertical_assignment = alia_vertical_assignment;
using main_axis_index = alia_main_axis_index;
using layout_node_vtable = alia_layout_node_vtable;

inline bool
has_content(line_requirements const& requirements)
{
    return requirements.height != 0 || requirements.ascent != 0
        || requirements.descent != 0;
}

inline bool
has_first_line_content(wrapping_requirements const& requirements)
{
    return has_content(requirements.first_line);
}

inline bool
has_wrapped_content(wrapping_requirements const& requirements)
{
    return has_content(requirements.last_line);
}

inline horizontal_requirements
measure_horizontal(measurement_context* ctx, layout_node* node)
{
    return node->vtable->measure_horizontal(ctx, node);
}

inline void
assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    return node->vtable->assign_widths(ctx, main_axis, node, assigned_width);
}

inline vertical_requirements
measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    return node->vtable->measure_vertical(
        ctx, main_axis, node, assigned_width);
}

inline void
assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    return node->vtable->assign_boxes(ctx, main_axis, node, box, baseline);
}

inline horizontal_requirements
measure_wrapped_horizontal(measurement_context* ctx, layout_node* node)
{
    return node->vtable->measure_wrapped_horizontal(ctx, node);
}

inline wrapping_requirements
measure_wrapped_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float current_x_offset,
    float line_width)
{
    return node->vtable->measure_wrapped_vertical(
        ctx, main_axis, node, current_x_offset, line_width);
}

inline void
assign_wrapped_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    wrapping_assignment const* assignment)
{
    return node->vtable->assign_wrapped_boxes(
        ctx, main_axis, node, assignment);
}

} // namespace alia
