#ifndef ALIA_UI_LAYOUT_COMPONENTS_H
#define ALIA_UI_LAYOUT_COMPONENTS_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/flags.h>

ALIA_EXTERN_C_BEGIN

// COMPOSITION CONTAINERS

// simple

void
alia_layout_column_begin(alia_context* ctx, alia_layout_flags_t flags);
void
alia_layout_column_end(alia_context* ctx);

void
alia_layout_row_begin(alia_context* ctx, alia_layout_flags_t flags);
void
alia_layout_row_end(alia_context* ctx);

void
alia_layout_flow_begin(alia_context* ctx, alia_layout_flags_t flags);
void
alia_layout_flow_end(alia_context* ctx);

void
alia_layout_block_flow_begin(alia_context* ctx, alia_layout_flags_t flags);
void
alia_layout_block_flow_end(alia_context* ctx);

// grid

struct alia_layout_grid_scope;
typedef alia_layout_grid_scope* alia_layout_grid_handle;

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
alia_layout_inset_begin(
    alia_context* ctx, alia_insets insets, alia_layout_flags_t flags);
void
alia_layout_inset_end(alia_context* ctx);

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

struct alia_layout_placement
{
    alia_box box;
    float baseline;
};

alia_layout_placement*
alia_layout_placement_hook_begin(alia_context* ctx, alia_layout_flags_t flags);
void
alia_layout_placement_hook_end(alia_context* ctx);

// LEAVES

// Emit a leaf node.
// This must be called on refresh passes (and ONLY on refresh passes).
void
alia_layout_leaf_emit(
    alia_context* ctx, alia_vec2f size, alia_layout_flags_t flags);

// Read the placement box for a leaf node.
// This must be called on NON-refresh passes (and never on refresh passes).
alia_box
alia_layout_leaf_read(alia_context* ctx);

ALIA_EXTERN_C_END

#endif // ALIA_UI_LAYOUT_COMPONENTS_H
