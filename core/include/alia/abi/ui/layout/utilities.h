#ifndef ALIA_LAYOUT_UTILITIES_H
#define ALIA_LAYOUT_UTILITIES_H

#include <alia/abi/base/geometry/types.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/style.h>

ALIA_EXTERN_C_BEGIN

// ARENA UTILITIES

typedef struct alia_layout_emission
{
    alia_bump_allocator arena;
    alia_layout_node** next_ptr;
} alia_layout_emission;

struct alia_layout_context
{
    alia_layout_emission emission;
    alia_bump_allocator placement;
};

static inline alia_bump_allocator*
alia_layout_node_arena(alia_context* ctx)
{
    // TODO: ALIA_ASSERT(alia_is_refresh_pass(ctx));
    return &ctx->layout->emission.arena;
}

static inline alia_bump_allocator*
alia_layout_placement_arena(alia_context* ctx)
{
    // TODO: ALIA_ASSERT(!alia_is_refresh_pass(ctx));
    return &ctx->layout->placement;
}

// COMPONENT-SIDE CONTAINER UTILITIES

struct alia_layout_container
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    alia_layout_node* first_child;
};

void
alia_layout_container_activate(
    alia_context* ctx, alia_layout_container* container);

void
alia_layout_container_deactivate(
    alia_context* ctx, alia_layout_container* container);

void
alia_layout_container_simple_begin(
    alia_context* ctx,
    alia_layout_node_vtable* vtable,
    alia_layout_flags_t flags);

void
alia_layout_container_simple_end(alia_context* ctx);

// PLACEMENT UTILITIES

typedef struct alia_layout_axis_placement
{
    float offset;
    float size;
} alia_layout_axis_placement;

// CONTAINER PLACEMENT - If no alignment flags are set, the node will be
// stretched to fill its assigned region.

// Resolve the horizontal placement for a container node.
alia_layout_axis_placement
alia_resolve_container_x(
    alia_layout_flags_t flags, float assigned_size, float required_size);

// Resolve the vertical placement for a container node.
alia_layout_axis_placement
alia_resolve_container_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent);

// Resolve the full 2D placement for a container node.
alia_box
alia_resolve_container_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent);

// LEAF PLACEMENT - Takes into account spacing. If no alignment flags are set,
// the node will be aligned to the start of its assigned region.

// Resolve the horizontal placement for a leaf node.
alia_layout_axis_placement
alia_resolve_leaf_x(
    alia_layout_flags_t flags,
    float assigned_size,
    float required_size,
    float spacing);

// Resolve the vertical placement for a leaf node.
alia_layout_axis_placement
alia_resolve_leaf_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float spacing);

// Resolve the full 2D placement for a leaf node.
alia_box
alia_resolve_leaf_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent,
    alia_vec2f spacing);

// Resolve the baseline offset for a node.
float
alia_resolve_baseline(
    alia_layout_flags_t flags,
    float assigned_height,
    float ascent,
    float descent);

// Resolve the growth factor for a node from its flags.
static inline float
alia_resolve_growth_factor(alia_layout_flags_t flags)
{
    return (flags & ALIA_GROW) ? 1.0f : 0.0f;
}

// Adjust the layout flags to fold the cross-axis flags into the appropriate
// axis-specific flags.
// Note that this invalidates other flags that aren't related to alignment,
// which is fine for what it's used for.
static inline alia_layout_flags_t
alia_fold_in_cross_axis_flags(
    alia_layout_flags_t flags, alia_main_axis_index main_axis)
{
    return flags | ((flags & ALIA_CROSS_ALIGNMENT_MASK) >> main_axis);
}

// DEFAULT NODE IMPLEMENTATIONS

alia_wrapping_requirements
alia_default_measure_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width);

// PROTOCOL INVOCATION UTILITIES

static inline alia_horizontal_requirements
alia_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    return node->vtable->measure_horizontal(ctx, node);
}

static inline void
alia_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    node->vtable->assign_widths(ctx, main_axis, node, assigned_width);
}

static inline alia_vertical_requirements
alia_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    return node->vtable->measure_vertical(
        ctx, main_axis, node, assigned_width);
}

static inline void
alia_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    node->vtable->assign_boxes(ctx, main_axis, node, box, baseline);
}

static inline alia_horizontal_requirements
alia_measure_wrapped_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    return node->vtable->measure_wrapped_horizontal(ctx, node);
}

static inline alia_wrapping_requirements
alia_measure_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width)
{
    return node->vtable->measure_wrapped_vertical(
        ctx, main_axis, node, current_x_offset, line_width);
}

static inline void
alia_assign_wrapped_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_wrapping_assignment const* assignment)
{
    node->vtable->assign_wrapped_boxes(ctx, main_axis, node, assignment);
}

// LINE WRAPPING UTILITIES

static inline bool
alia_layout_line_has_content(alia_line_requirements const& line)
{
    return line.height != 0 || line.ascent != 0 || line.descent != 0;
}

static void
alia_layout_line_reset(alia_line_requirements& line)
{
    line.height = 0;
    line.ascent = 0;
    line.descent = 0;
}

static void
alia_layout_line_fold_in_child(
    alia_line_requirements& line, alia_vertical_requirements const& child)
{
    line.height = alia_max(line.height, child.min_size);
    line.ascent = alia_max(line.ascent, child.ascent);
    line.descent = alia_max(line.descent, child.descent);
}

static void
alia_layout_line_finalize_height(alia_line_requirements& line)
{
    line.height = alia_max(line.height, line.ascent + line.descent);
}

static inline bool
alia_layout_wrapping_has_first_line_content(
    alia_wrapping_requirements const& requirements)
{
    return alia_layout_line_has_content(requirements.first_line);
}

static inline bool
alia_layout_wrapping_has_wrapped_content(
    alia_wrapping_requirements const& requirements)
{
    return alia_layout_line_has_content(requirements.last_line);
}

ALIA_EXTERN_C_END

#endif // ALIA_LAYOUT_UTILITIES_H
