#pragma once

#include <cstdint>

#include <alia/flow/infinite_arena.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/api.hpp>

namespace alia {

struct LayoutNodeVtable;

enum class LayoutAlignment : uint8_t
{
    Start,
    Center,
    End,
    Baseline,
    Fill,
};

struct LayoutProperties
{
    LayoutAlignment x_alignment;
    LayoutAlignment y_alignment;
    uint8_t growth_factor;
};

struct LayoutNode
{
    LayoutNodeVtable* vtable;
    LayoutNode* next_sibling;
};

struct LayoutContainer
{
    LayoutNode base;
    LayoutProperties props;
    uint32_t child_count;
    LayoutNode* first_child;
};

using LayoutSpecArena = HeterogeneousInfiniteArena;

using LayoutScratchArena = HeterogeneousInfiniteArena;

using LayoutPlacementArena = HeterogeneousInfiniteArena;

struct LayoutPlacementNode
{
    LayoutPlacementNode* next;
};

struct LeafLayoutPlacement
{
    LayoutPlacementNode base;
    Vec2 position;
    Vec2 size;
};

struct HorizontalRequirements
{
    float min_size;
    float growth_factor = 0.0f;
};

struct VerticalRequirements
{
    float min_size;
    float growth_factor = 0.0f;
    float ascent = 0.0f;
    float descent = 0.0f;
};

struct PlacementContext
{
    LayoutScratchArena* scratch;
    LayoutPlacementArena* arena;
    LayoutPlacementNode** next_ptr;
};

struct MeasurementContext
{
    LayoutScratchArena* scratch;
};

struct WrappingRequirements
{
    float line_height;
    float ascent;
    float descent;
    // TODO: Combine `wrap_count` and `wrapped_immediately` into a single
    // uint32_t.
    int wrap_count;
    bool wrapped_immediately;
    float new_x_offset;
};

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

    // vertical assignment for middle lines - to be used for content on
    // intermediate lines (i.e., where this node completely fills the line)
    VerticalAssignment middle_lines;

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

HorizontalRequirements
default_measure_wrapped_horizontal(MeasurementContext* ctx, LayoutNode* node);

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

WrappingRequirements
default_measure_wrapped_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width);

inline void
assign_wrapped_boxes(
    PlacementContext* ctx,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    return node->vtable->assign_wrapped_boxes(ctx, node, assignment);
}

LayoutPlacementNode*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space);

// TODO: Move to some utilities file...

struct LayoutAxisPlacement
{
    float offset;
    float size;
};

LayoutAxisPlacement
resolve_axis_assignment(
    LayoutAlignment alignment,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent);

Box
resolve_assignment(
    LayoutProperties props,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent);

} // namespace alia
