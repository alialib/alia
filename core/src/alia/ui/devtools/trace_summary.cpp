#include <alia/abi/ui/devtools/trace_summary.h>

#include <alia/abi/prelude.h>

#include <algorithm>
#include <cstring>

extern "C" {

alia_trace_summary_config
alia_trace_summary_config_defaults(void)
{
    return alia_trace_summary_config{.spike_ratio = 1.5f, .max_spikes = 5};
}

int64_t
alia_trace_frame_wall_us(alia_trace_frame const* frame)
{
    if (!frame)
        return 0;
    return alia_trace_ticks_to_ns(frame->wall_end - frame->wall_start) / 1000;
}

void
alia_trace_summary_state_init(alia_trace_summary_state* state)
{
    ALIA_ASSERT(state);
    state->sticky_frame_index = ALIA_TRACE_SUMMARY_FRAME_INDEX_NONE;
}

} // extern "C"

namespace {

bool
slot_is_live(alia_ui_tracer const* tracer, uint32_t slot)
{
    if (tracer->stored_count == 0)
        return false;
    if (tracer->stored_count < ALIA_TRACE_RING_SIZE)
        return slot < tracer->stored_count;
    return true;
}

alia_trace_frame const*
find_frame_by_index(alia_ui_tracer const* tracer, uint64_t index)
{
    for (uint32_t i = 0; i < ALIA_TRACE_RING_SIZE; ++i)
    {
        if (!slot_is_live(tracer, i))
            continue;
        if (tracer->frames[i].index == index)
            return &tracer->frames[i];
    }
    return nullptr;
}

bool
is_spike(int64_t wall_us, int64_t threshold_us)
{
    return wall_us > threshold_us;
}

void
set_sticky_representative(
    alia_trace_summary_state* state, alia_trace_frame const* frame)
{
    if (frame)
        state->sticky_frame_index = frame->index;
    else
        state->sticky_frame_index = ALIA_TRACE_SUMMARY_FRAME_INDEX_NONE;
}

} // namespace

extern "C" {

alia_trace_summary
alia_trace_summarize(
    alia_ui_tracer const* tracer,
    alia_trace_summary_config const* config,
    alia_trace_summary_state* state)
{
    ALIA_ASSERT(tracer);
    ALIA_ASSERT(state);

    alia_trace_summary_config const cfg
        = config ? *config : alia_trace_summary_config_defaults();

    alia_trace_summary summary{};

    if (tracer->stored_count == 0)
    {
        set_sticky_representative(state, nullptr);
        return summary;
    }

    int64_t sum_us = 0;
    uint32_t live_count = 0;
    for (uint32_t i = 0; i < ALIA_TRACE_RING_SIZE; ++i)
    {
        if (!slot_is_live(tracer, i))
            continue;
        sum_us += alia_trace_frame_wall_us(&tracer->frames[i]);
        ++live_count;
    }
    ALIA_ASSERT(live_count > 0);
    summary.average_us = sum_us / static_cast<int64_t>(live_count);

    float const ratio = cfg.spike_ratio > 0.f ? cfg.spike_ratio : 1.5f;
    summary.threshold_us = static_cast<int64_t>(
        static_cast<double>(summary.average_us) * ratio);

    uint32_t max_spikes = cfg.max_spikes;
    if (max_spikes == 0)
        max_spikes = 1;
    if (max_spikes > ALIA_TRACE_SUMMARY_MAX_SPIKES)
        max_spikes = ALIA_TRACE_SUMMARY_MAX_SPIKES;

    alia_trace_frame const* candidates[ALIA_TRACE_RING_SIZE];
    uint32_t candidate_count = 0;
    for (uint32_t i = 0; i < ALIA_TRACE_RING_SIZE; ++i)
    {
        if (!slot_is_live(tracer, i))
            continue;
        alia_trace_frame const* frame = &tracer->frames[i];
        if (is_spike(alia_trace_frame_wall_us(frame), summary.threshold_us))
        {
            candidates[candidate_count++] = frame;
        }
    }

    std::sort(
        candidates,
        candidates + candidate_count,
        [](alia_trace_frame const* a, alia_trace_frame const* b) {
            return alia_trace_frame_wall_us(a) > alia_trace_frame_wall_us(b);
        });

    summary.spike_count = (std::min) (candidate_count, max_spikes);
    for (uint32_t i = 0; i < summary.spike_count; ++i)
        summary.spikes[i] = candidates[i];
    if (candidate_count > max_spikes)
        summary.spike_overflow = candidate_count - max_spikes;

    auto pick_first_non_spike = [&]() -> alia_trace_frame const* {
        for (uint32_t i = 0; i < ALIA_TRACE_RING_SIZE; ++i)
        {
            if (!slot_is_live(tracer, i))
                continue;
            alia_trace_frame const* frame = &tracer->frames[i];
            if (!is_spike(
                    alia_trace_frame_wall_us(frame), summary.threshold_us))
                return frame;
        }
        return nullptr;
    };

    if (state->sticky_frame_index != ALIA_TRACE_SUMMARY_FRAME_INDEX_NONE)
    {
        alia_trace_frame const* sticky
            = find_frame_by_index(tracer, state->sticky_frame_index);
        if (sticky
            && !is_spike(
                alia_trace_frame_wall_us(sticky), summary.threshold_us))
        {
            summary.representative = sticky;
        }
        else
        {
            summary.representative = pick_first_non_spike();
            set_sticky_representative(state, summary.representative);
        }
    }
    else
    {
        summary.representative = pick_first_non_spike();
        set_sticky_representative(state, summary.representative);
    }

    return summary;
}

} // extern "C"
