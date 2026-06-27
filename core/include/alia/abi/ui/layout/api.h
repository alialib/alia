#ifndef ALIA_ABI_UI_LAYOUT_API_H
#define ALIA_ABI_UI_LAYOUT_API_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/flags.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_context alia_context;

typedef struct alia_layout_content_metrics
{
    alia_vec2f size;
    float ascent;
    float descent;
} alia_layout_content_metrics;

static inline alia_layout_content_metrics
alia_layout_content_metrics_make(alia_vec2f size)
{
    return ALIA_BRACED_INIT(alia_layout_content_metrics, size, 0.f, 0.f);
}

// COMPOSITION CONTAINERS

// simple

void
alia_layout_column_begin(
    alia_context* ctx, alia_layout_flags_t flags, float gap);
void
alia_layout_column_end(alia_context* ctx);

void
alia_layout_row_begin(alia_context* ctx, alia_layout_flags_t flags, float gap);
void
alia_layout_row_end(alia_context* ctx);

void
alia_layout_flow_begin(
    alia_context* ctx,
    alia_layout_flags_t flags,
    float gap,
    float line_gap,
    float minimum_line_height);
void
alia_layout_flow_end(alia_context* ctx);

void
alia_layout_block_flow_begin(
    alia_context* ctx,
    alia_layout_flags_t flags,
    float gap,
    float line_gap,
    float minimum_line_height);
void
alia_layout_block_flow_end(alia_context* ctx);

// grid

typedef struct alia_layout_grid_scope* alia_layout_grid_handle;

alia_layout_grid_handle
alia_layout_grid_begin(alia_context* ctx, alia_layout_flags_t flags);
void
alia_layout_grid_end(alia_context* ctx);

void
alia_layout_grid_row_begin(
    alia_context* ctx,
    alia_layout_grid_handle grid,
    alia_layout_flags_t flags);
void
alia_layout_grid_row_end(alia_context* ctx);

// WRAPPERS

void
alia_layout_edge_offsets_begin(
    alia_context* ctx, alia_edge_offsets offsets, alia_layout_flags_t flags);
void
alia_layout_edge_offsets_end(alia_context* ctx);

void
alia_layout_alignment_override_begin(
    alia_context* ctx, alia_layout_flags_t flags);
void
alia_layout_alignment_override_end(alia_context* ctx);

void
alia_layout_growth_override_begin(alia_context* ctx, float growth);
void
alia_layout_growth_override_end(alia_context* ctx);

void
alia_layout_min_size_begin(alia_context* ctx, alia_vec2f min_size);
void
alia_layout_min_size_end(alia_context* ctx);

// LEAVES

// Emit a leaf node.
// This must be called on refresh passes (and ONLY on refresh passes).
void
alia_layout_leaf_emit(
    alia_context* ctx,
    alia_layout_content_metrics content,
    alia_layout_flags_t flags);

// Emit a zero-height flow spring that absorbs extra horizontal space during
// line justification. This must be called on refresh passes (and ONLY on
// refresh passes).
void
alia_layout_flow_spring_emit(alia_context* ctx, float min_width);

// BOX CONSUMPTION

// Consume the placement box for a node.
// This must be called on NON-refresh passes (and never on refresh passes).
alia_box
alia_layout_consume_box(alia_context* ctx);

typedef struct alia_layout_box_array
{
    uint32_t count;
    alia_box* boxes;
} alia_layout_box_array;

// Some nodes may occupy a region that can't be described by a single box. For
// example, an edge_offsets node inside a flow panel may wrap with its content.
// If asked to provide its box back to the component code, it will emit an
// array of boxes.
alia_layout_box_array
alia_layout_consume_box_array(alia_context* ctx);

ALIA_EXTERN_C_END

#include <alia/abi/ui/layout/system.h>

#endif // ALIA_ABI_UI_LAYOUT_API_H
