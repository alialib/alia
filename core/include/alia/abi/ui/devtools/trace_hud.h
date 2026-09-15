#ifndef ALIA_ABI_UI_DEVTOOLS_TRACE_HUD_H
#define ALIA_ABI_UI_DEVTOOLS_TRACE_HUD_H

#include <alia/abi/ui/devtools/trace_summary.h>

#include <stdbool.h>
#include <stdint.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_context alia_context;
typedef struct alia_resolved_font alia_resolved_font;

// typography for the trace overlay HUD
typedef struct alia_trace_hud_style
{
    alia_resolved_font const* font;
} alia_trace_hud_style;

// one spike row shown in the HUD (live top-K and/or sticky-expanded)
typedef struct alia_trace_hud_spike_row
{
    // full frame snapshot - Event passes must not read the live ring; the
    // ring can overwrite this frame between refreshes and change pass_count.
    alia_trace_frame frame;
    bool expanded;
    // true when this row is kept only because it is still expanded after
    // leaving the live top-K list
    bool pinned;
} alia_trace_hud_spike_row;

#ifndef ALIA_TRACE_HUD_MAX_ROWS
// live top-K spikes plus the same number of sticky-expanded leftovers
#define ALIA_TRACE_HUD_MAX_ROWS (ALIA_TRACE_SUMMARY_MAX_SPIKES * 2)
#endif

// persistent overlay UI state for a trace HUD instance
typedef struct alia_trace_hud_view_state
{
    alia_trace_summary_state summary_state;

    // spike rows captured on the last refresh - Visit order and body shape
    // are frozen between refreshes so placement replay stays aligned.
    alia_trace_hud_spike_row spikes[ALIA_TRACE_HUD_MAX_ROWS];
    uint32_t spike_count;
    uint32_t spike_overflow;

    // representative frame snapshot from the last refresh
    alia_trace_frame representative;
    bool has_representative;
    bool avg_expanded;
} alia_trace_hud_view_state;

void
alia_trace_hud_view_state_init(alia_trace_hud_view_state* state);

// Draw the trace overlay HUD.
// Returns true when expand state changed.
bool
alia_ui_trace_hud(
    alia_context* ctx,
    alia_ui_tracer const* tracer,
    alia_trace_hud_view_state* state,
    alia_trace_hud_style const* style,
    alia_trace_summary_config const* config);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DEVTOOLS_TRACE_HUD_H */
