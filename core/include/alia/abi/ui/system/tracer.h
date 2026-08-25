#ifndef ALIA_ABI_UI_SYSTEM_TRACER_H
#define ALIA_ABI_UI_SYSTEM_TRACER_H

#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/trace.h>

ALIA_EXTERN_C_BEGIN

#ifndef ALIA_TRACE_MAX_PASSES
#define ALIA_TRACE_MAX_PASSES 32
#endif

#ifndef ALIA_TRACE_RING_SIZE
#define ALIA_TRACE_RING_SIZE 128
#endif

// trace info for a single frame
typedef struct alia_trace_frame
{
    uint64_t index;
    alia_trace_tick wall_start;
    alia_trace_tick wall_end;
    alia_trace_pass passes[ALIA_TRACE_MAX_PASSES];
    uint16_t pass_count;
    // count of passes dropped because the per-frame buffer was full
    uint16_t dropped_passes;
} alia_trace_frame;

// ring-buffer collector that implements the `alia_ui_trace_sink` interface
typedef struct alia_ui_tracer
{
    alia_ui_trace_sink sink;
    alia_trace_frame frames[ALIA_TRACE_RING_SIZE];
    uint32_t write_index;
    uint32_t stored_count;
    uint64_t next_frame_index;
    bool in_frame;
    alia_trace_frame* current;
} alia_ui_tracer;

void
alia_ui_tracer_init(alia_ui_tracer* tracer);

// Install `tracer` as the UI system's trace sink. The tracer must outlive the
// system (or until `alia_ui_tracer_detach`).
void
alia_ui_tracer_attach(alia_ui_tracer* tracer, alia_ui_system* ui);

void
alia_ui_tracer_detach(alia_ui_tracer* tracer, alia_ui_system* ui);

// Latest completed frame, or null if none yet.
alia_trace_frame const*
alia_ui_tracer_latest(alia_ui_tracer const* tracer);

// Frame `steps` before the latest (0 = latest). Null if out of range.
alia_trace_frame const*
alia_ui_tracer_frame_ago(alia_ui_tracer const* tracer, uint32_t steps);

// Short label for a pass kind (static string).
char const*
alia_trace_pass_kind_name(alia_trace_pass_kind kind);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_SYSTEM_TRACER_H */
