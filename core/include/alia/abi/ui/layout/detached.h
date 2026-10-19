#ifndef ALIA_ABI_UI_LAYOUT_DETACHED_H
#define ALIA_ABI_UI_LAYOUT_DETACHED_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/placement.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_context alia_ui_context;
typedef struct alia_layout_node alia_layout_node;

// persistent state for a detached layout tree - Zero-initialization is valid.
typedef struct alia_layout_detached
{
    // root of the detached layout tree
    alia_layout_node* root;
    // placement region written by the last assign for `generation`
    alia_layout_placement_region region;
    // placement generation for the last assignment
    alia_layout_placement_generation generation;
    // size from the last assignment
    alia_vec2f size;
} alia_layout_detached;

// Typical usage:
//
//   Refresh pass:
//     alia_layout_detached_begin(ctx, &detached);
//       /* emit layout nodes */
//     alia_layout_detached_end(ctx);
//
//   After the main layout resolve, on non-refresh passes:
//     alia_vec2f preferred = alia_layout_detached_measure(ctx, &detached);
//     alia_vec2f assigned = /* clamp / place using preferred */;
//     alia_layout_detached_assign(ctx, &detached, assigned);
//     alia_layout_detached_begin(ctx, &detached);
//       /* consume placement boxes */
//     alia_layout_detached_end(ctx);
//
// `assign` must be called before `begin` on non-refresh passes. It may be
// called once per placement generation. Further calls in the same generation
// are no-ops. Nesting is supported.

// Measure the *preferred* size of the detached tree.
// (The "preferred" size differs from the normal minimum size in that
// shrinkable widgets (e.g., scrollviews) will report their actual content
// size.)
alia_vec2f
alia_layout_detached_measure(
    alia_ui_context* ctx, alia_layout_detached* detached);

// Assign the detached tree into the shared placement arena at (0, 0) with
// `size`.
// Call this on non-refresh passes before `alia_layout_detached_begin`.
// This caches the result for the current placement generation.
void
alia_layout_detached_assign(
    alia_ui_context* ctx, alia_layout_detached* detached, alia_vec2f size);

// Get the size from the last successful assign.
// (This is zeros if never assigned.)
static inline alia_vec2f
alia_layout_detached_size(alia_layout_detached const* detached)
{
    ALIA_ASSERT(detached);
    return detached->size;
}

// Begin a detached layout scope.
// On refresh, this redirects layout emission to `detached->root`.
// On other passes, it pushes `detached`'s placement region for consumption.
// Nesting is supported.
// Note that on non-refresh passes, `alia_layout_detached_assign` must have
// already been called for the current placement generation.
void
alia_layout_detached_begin(alia_ui_context* ctx, alia_layout_detached* detached);

void
alia_layout_detached_end(alia_ui_context* ctx);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_DETACHED_H
