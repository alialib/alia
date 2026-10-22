#ifndef ALIA_ABI_UI_CONTEXT_IDS_H
#define ALIA_ABI_UI_CONTEXT_IDS_H

#include <alia/abi/ui/context.h>

ALIA_EXTERN_C_BEGIN

static inline alia_id_view
alia_context_metrics_id(alia_ui_context const* ctx)
{
    ALIA_ASSERT(ctx);
    return ctx->id.metrics;
}

static inline alia_id_view
alia_context_paint_id(alia_ui_context const* ctx)
{
    ALIA_ASSERT(ctx);
    return ctx->id.paint;
}

// Make `id` the metrics ID until the matching pop.
void
alia_context_metrics_id_push(alia_ui_context* ctx, alia_id_view id);

// Restore the metrics ID from the matching push.
void
alia_context_metrics_id_pop(alia_ui_context* ctx);

// Make `id` the paint ID until the matching pop.
void
alia_context_paint_id_push(alia_ui_context* ctx, alia_id_view id);

// Restore the paint ID from the matching push.
void
alia_context_paint_id_pop(alia_ui_context* ctx);

// Begin a scope in which the context IDs are collapsed to single revision
// counters. For each ID (metrics and paint), this caches the (possibly large)
// ID that was active on the context, scans it for changes (on refresh passes),
// and increments a revision counter whenever one is detected.
// The revision counter is then published as the ID for the scope.
void
alia_context_id_collapse_begin(alia_ui_context* ctx);

// Restore the metrics and paint IDs from the matching begin.
void
alia_context_id_collapse_end(alia_ui_context* ctx);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_CONTEXT_IDS_H */
