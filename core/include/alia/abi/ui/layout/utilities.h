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

int
alia_default_count_flow_fragments(
    alia_measurement_context* ctx, alia_layout_node* node);

void
alia_default_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter);

void
alia_default_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader);

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

static inline int
alia_count_flow_fragments(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    return node->vtable->count_flow_fragments(ctx, node);
}

static inline void
alia_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    node->vtable->emit_flow_fragments(ctx, node, emitter);
}

static inline void
alia_layout_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    return node->vtable->read_fragment_placements(ctx, node, reader);
}

// FLOW FRAGMENT UTILITIES

static inline void
alia_layout_emit_flow_fragment(
    alia_flow_fragment_emitter* emitter, alia_flow_fragment fragment)
{
    emitter->fragments[emitter->index] = fragment;
    ++emitter->index;
}

static inline alia_flow_fragment const*
alia_layout_read_fragment_spec(alia_flow_fragment_reader const* reader)
{
    return &reader->fragments[reader->index];
}

static inline alia_flow_fragment_placement const*
alia_layout_read_fragment_placement(alia_flow_fragment_reader const* reader)
{
    return &reader->placements[reader->index];
}

static inline void
alia_layout_advance_fragment(alia_flow_fragment_reader* reader)
{
    ++reader->index;
}

// LINE WRAPPING UTILITIES

static inline alia_line_requirements
alia_layout_line_from_assignment(alia_vertical_assignment const& assignment)
{
    return ALIA_BRACED_INIT(
        alia_line_requirements,
        assignment.line_height,
        assignment.baseline_offset,
        assignment.line_height - assignment.baseline_offset);
}

static inline bool
alia_layout_line_has_content(alia_line_requirements const& line)
{
    return line.height != 0 || line.ascent != 0 || line.descent != 0;
}

static inline void
alia_layout_line_reset(alia_line_requirements& line)
{
    line.height = 0;
    line.ascent = 0;
    line.descent = 0;
}

static inline void
alia_layout_line_fold_in_line(
    alia_line_requirements& line, alia_line_requirements const& other)
{
    line.height = alia_max(line.height, other.height);
    line.ascent = alia_max(line.ascent, other.ascent);
    line.descent = alia_max(line.descent, other.descent);
}

static inline void
alia_layout_line_fold_in_assignment(
    alia_line_requirements& line, alia_vertical_assignment const& assignment)
{
    line.height = alia_max(line.height, assignment.line_height);
    line.ascent = alia_max(line.ascent, assignment.baseline_offset);
    line.descent = alia_max(
        line.descent, assignment.line_height - assignment.baseline_offset);
}

static inline void
alia_layout_line_fold_in_child(
    alia_line_requirements& line, alia_vertical_requirements const& child)
{
    line.height = alia_max(line.height, child.min_size);
    line.ascent = alia_max(line.ascent, child.ascent);
    line.descent = alia_max(line.descent, child.descent);
}

static inline void
alia_layout_line_fold_in_fragment(
    alia_line_requirements& line, alia_flow_fragment const& fragment)
{
    line.height = alia_max(line.height, fragment.height);
    line.ascent = alia_max(line.ascent, fragment.ascent);
    line.descent = alia_max(line.descent, fragment.descent);
}

static inline float
alia_layout_line_final_height(alia_line_requirements const& line)
{
    return alia_max(line.height, line.ascent + line.descent);
}

static inline void
alia_layout_line_finalize_height(alia_line_requirements& line)
{
    line.height = alia_layout_line_final_height(line);
}

typedef struct alia_layout_line_spacing
{
    float leading;
    float gap;
} alia_layout_line_spacing;

alia_layout_line_spacing
alia_layout_justify_line(
    alia_layout_flags_t flags, float extra_space, int count);

ALIA_EXTERN_C_END

#endif // ALIA_LAYOUT_UTILITIES_H
