#pragma once

#include <alia/flow/infinite_arena.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

struct LayoutNodeVtable;

struct LayoutNode
{
    LayoutNodeVtable* vtable;
    LayoutNode* next_sibling;
};

struct HorizontalRequirements
{
    float min_size;
    float growth_factor;
};

struct VerticalRequirements
{
    float min_size;
    float growth_factor;
    float ascent;
    float descent;
};

struct PlacementContext
{
    InfiniteArena* scratch;
    InfiniteArena* arena;
};

struct MeasurementContext
{
    InfiniteArena* scratch;
};

struct LineRequirements
{
    float height;
    float ascent;
    float descent;
};

struct WrappingRequirements
{
    // the child's contribution to the line that was already in progress when
    // it was invoked - This may be all 0s if the child doesn't actually place
    // anything on that line.
    LineRequirements first_line;
    // the total height that the child uses in between the first line and the
    // last line
    float interior_height;
    // the child's contribution to the last line that it places content on -
    // This should be all 0s if the child never wraps.
    LineRequirements last_line;
    // the X offset at which the child's content ends
    float end_x;
};

inline bool
has_content(LineRequirements const& requirements)
{
    return requirements.height != 0 || requirements.ascent != 0
        || requirements.descent != 0;
}

inline bool
has_first_line_content(WrappingRequirements const& requirements)
{
    return has_content(requirements.first_line);
}

inline bool
has_wrapped_content(WrappingRequirements const& requirements)
{
    return has_content(requirements.last_line);
}

struct VerticalAssignment
{
    float line_height;
    float baseline_offset;
};

struct WrappingAssignment
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
    VerticalAssignment first_line;

    // vertical assignment for last line - to be used for content on the last
    // line (i.e., where later nodes might share the same line)
    VerticalAssignment last_line;
};

struct LayoutNodeVtable
{
    HorizontalRequirements (*measure_horizontal)(
        MeasurementContext* ctx, LayoutNode* node);

    void (*assign_widths)(
        MeasurementContext* ctx, LayoutNode* node, float assigned_width);

    VerticalRequirements (*measure_vertical)(
        MeasurementContext* ctx, LayoutNode* node, float assigned_width);

    void (*assign_boxes)(
        PlacementContext* ctx, LayoutNode* node, Box box, float baseline);

    HorizontalRequirements (*measure_wrapped_horizontal)(
        MeasurementContext* ctx, LayoutNode* node);

    WrappingRequirements (*measure_wrapped_vertical)(
        MeasurementContext* ctx,
        LayoutNode* node,
        float current_x_offset,
        float line_width);

    void (*assign_wrapped_boxes)(
        PlacementContext* ctx,
        LayoutNode* node,
        WrappingAssignment const* assignment);
};

inline HorizontalRequirements
measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    return node->vtable->measure_horizontal(ctx, node);
}

inline void
assign_widths(MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    return node->vtable->assign_widths(ctx, node, assigned_width);
}

inline VerticalRequirements
measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    return node->vtable->measure_vertical(ctx, node, assigned_width);
}

inline void
assign_boxes(PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    return node->vtable->assign_boxes(ctx, node, box, baseline);
}

inline HorizontalRequirements
measure_wrapped_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    return node->vtable->measure_wrapped_horizontal(ctx, node);
}

inline WrappingRequirements
measure_wrapped_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    return node->vtable->measure_wrapped_vertical(
        ctx, node, current_x_offset, line_width);
}

inline void
assign_wrapped_boxes(
    PlacementContext* ctx,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    return node->vtable->assign_wrapped_boxes(ctx, node, assignment);
}

} // namespace alia
