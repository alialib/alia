#pragma once

#include <alia/abi/arena.h>
#include <alia/abi/geometry.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alia_layout_node_vtable alia_layout_node_vtable;

typedef struct alia_layout_node
{
    alia_layout_node_vtable* vtable;
    alia_layout_node* next_sibling;
} alia_layout_node;

typedef struct alia_horizontal_requirements
{
    float min_size;
    float growth_factor;
} alia_horizontal_requirements;

typedef struct alia_vertical_requirements
{
    float min_size;
    float growth_factor;
    float ascent;
    float descent;
} alia_vertical_requirements;

typedef struct alia_placement_context
{
    alia_arena_view* scratch;
    alia_arena_view* arena;
} alia_placement_context;

typedef struct alia_measurement_context
{
    alia_arena_view* scratch;
} alia_measurement_context;

typedef struct alia_line_requirements
{
    float height;
    float ascent;
    float descent;
} alia_line_requirements;

typedef struct alia_wrapping_requirements
{
    // the child's contribution to the line that was already in progress when
    // it was invoked - This may be all 0s if the child doesn't actually place
    // anything on that line.
    alia_line_requirements first_line;

    // the total height that the child uses in between the first line and the
    // last line
    float interior_height;

    // the child's contribution to the last line that it places content on -
    // This should be all 0s if the child never wraps.
    alia_line_requirements last_line;

    // the X offset at which the child's content ends
    float end_x;
} alia_wrapping_requirements;

typedef struct alia_vertical_assignment
{
    float line_height;
    float baseline_offset;
} alia_vertical_assignment;

typedef struct alia_wrapping_assignment
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
    alia_vertical_assignment first_line;

    // vertical assignment for last line - to be used for content on the last
    // line (i.e., where later nodes might share the same line)
    alia_vertical_assignment last_line;
} alia_wrapping_assignment;

typedef uint8_t alia_main_axis_index;
#define ALIA_MAIN_AXIS_X 3
#define ALIA_MAIN_AXIS_Y 6

typedef struct alia_layout_node_vtable
{
    alia_horizontal_requirements (*measure_horizontal)(
        alia_measurement_context* ctx, alia_layout_node* node);

    void (*assign_widths)(
        alia_measurement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        float assigned_width);

    alia_vertical_requirements (*measure_vertical)(
        alia_measurement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        float assigned_width);

    void (*assign_boxes)(
        alia_placement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        alia_box box,
        float baseline);

    alia_horizontal_requirements (*measure_wrapped_horizontal)(
        alia_measurement_context* ctx, alia_layout_node* node);

    alia_wrapping_requirements (*measure_wrapped_vertical)(
        alia_measurement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        float current_x_offset,
        float line_width);

    void (*assign_wrapped_boxes)(
        alia_placement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        alia_wrapping_assignment const* assignment);

} alia_layout_node_vtable;

#ifdef __cplusplus
}
#endif
