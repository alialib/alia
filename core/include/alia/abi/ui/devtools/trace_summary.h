#ifndef ALIA_ABI_UI_DEVTOOLS_TRACE_SUMMARY_H
#define ALIA_ABI_UI_DEVTOOLS_TRACE_SUMMARY_H

#include <alia/abi/ui/system/tracer.h>

#include <stdbool.h>
#include <stdint.h>

ALIA_EXTERN_C_BEGIN

#ifndef ALIA_TRACE_SUMMARY_MAX_SPIKES
#define ALIA_TRACE_SUMMARY_MAX_SPIKES 5
#endif

// configuration for spike detection and list size
typedef struct alia_trace_summary_config
{
    // A frame is considered a spike if the ratio of its wall time to the
    // average frame wall time exceeds `spike_ratio`.
    float spike_ratio;
    uint32_t max_spikes;
} alia_trace_summary_config;

alia_trace_summary_config
alia_trace_summary_config_defaults(void);

// derived snapshot of a tracer ring buffer
typedef struct alia_trace_summary
{
    int64_t average_us;
    int64_t threshold_us;
    alia_trace_frame const* representative;
    alia_trace_frame const* spikes[ALIA_TRACE_SUMMARY_MAX_SPIKES];
    uint32_t spike_count;
    // count of spikes that were dropped because the buffer was full
    uint32_t spike_overflow;
} alia_trace_summary;

#define ALIA_TRACE_SUMMARY_FRAME_INDEX_NONE UINT64_MAX

// persistent state across `alia_trace_summarize` calls
typedef struct alia_trace_summary_state
{
    // sticky representative frame index -
    // `ALIA_TRACE_SUMMARY_FRAME_INDEX_NONE` when unset
    uint64_t sticky_frame_index;
} alia_trace_summary_state;

int64_t
alia_trace_frame_wall_us(alia_trace_frame const* frame);

void
alia_trace_summary_state_init(alia_trace_summary_state* state);

// Calculate a summary of `tracer`. If `config` is null, a default is used.
// `state` is internal state that should be maintained across calls if you want
// the summary to remain stable. (It must not be null.)
alia_trace_summary
alia_trace_summarize(
    alia_ui_tracer const* tracer,
    alia_trace_summary_config const* config,
    alia_trace_summary_state* state);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DEVTOOLS_TRACE_SUMMARY_H */
