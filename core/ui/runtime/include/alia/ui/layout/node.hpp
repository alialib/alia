#pragma once

#include <alia/kernel/infinite_arena.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

struct layout_node_vtable;

struct layout_node
{
    layout_node_vtable* vtable;
    layout_node* next_sibling;
};

struct horizontal_requirements
{
    float min_size;
    float growth_factor;
};

struct vertical_requirements
{
    float min_size;
    float growth_factor;
    float ascent;
    float descent;
};

struct placement_context
{
    infinite_arena* scratch;
    infinite_arena* arena;
};

struct measurement_context
{
    infinite_arena* scratch;
};

struct line_requirements
{
    float height;
    float ascent;
    float descent;
};

struct wrapping_requirements
{
    // the child's contribution to the line that was already in progress when
    // it was invoked - This may be all 0s if the child doesn't actually place
    // anything on that line.
    line_requirements first_line;
    // the total height that the child uses in between the first line and the
    // last line
    float interior_height;
    // the child's contribution to the last line that it places content on -
    // This should be all 0s if the child never wraps.
    line_requirements last_line;
    // the X offset at which the child's content ends
    float end_x;
};

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

struct vertical_assignment
{
    float line_height;
    float baseline_offset;
};

struct wrapping_assignment
{
    // X position of the flow layout
    float x_base;

    // width of the flow layout
    float line_width;

    // X offset of the first line - relative to `x_base`; to be used for any
    // content that fits before any wrapping occurs
    float first_line_x_offset;

    // Y position of the first line
    float y_base;

    // vertical assignment for first line - to be used for any content that
    // fits before any wrapping occurs
    vertical_assignment first_line;

    // vertical assignment for last line - to be used for content on the last
    // line (i.e., where later nodes might share the same line)
    vertical_assignment last_line;
};

using main_axis_index = std::uint8_t;
constexpr main_axis_index MAIN_AXIS_X = 3;
constexpr main_axis_index MAIN_AXIS_Y = 6;

struct layout_node_vtable
{
    horizontal_requirements (*measure_horizontal)(
        measurement_context* ctx, layout_node* node);

    void (*assign_widths)(
        measurement_context* ctx,
        main_axis_index main_axis,
        layout_node* node,
        float assigned_width);

    vertical_requirements (*measure_vertical)(
        measurement_context* ctx,
        main_axis_index main_axis,
        layout_node* node,
        float assigned_width);

    void (*assign_boxes)(
        placement_context* ctx,
        main_axis_index main_axis,
        layout_node* node,
        box box,
        float baseline);

    horizontal_requirements (*measure_wrapped_horizontal)(
        measurement_context* ctx, layout_node* node);

    wrapping_requirements (*measure_wrapped_vertical)(
        measurement_context* ctx,
        main_axis_index main_axis,
        layout_node* node,
        float current_x_offset,
        float line_width);

    void (*assign_wrapped_boxes)(
        placement_context* ctx,
        main_axis_index main_axis,
        layout_node* node,
        wrapping_assignment const* assignment);
};

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
