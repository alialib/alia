#ifndef ALIA_CORE_TIMING_TIMER_HPP
#define ALIA_CORE_TIMING_TIMER_HPP

#include <alia/core/context/interface.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/system/internals.hpp>
#include <alia/core/timing/ticks.hpp>

// This file defines the interface to timer events in alia.

namespace alia {

struct timer_data
{
    component_identity identity;
    bool active = false;
    millisecond_count expected_trigger_time;
};

void
schedule_timer_event(
    dataless_core_context ctx,
    external_component_id id,
    millisecond_count trigger_time);

bool
detect_timer_event(dataless_core_context ctx, timer_data& data);

void
start_timer(
    dataless_core_context ctx, timer_data& data, millisecond_count duration);

void
restart_timer(
    dataless_core_context ctx, timer_data& data, millisecond_count duration);

struct timer
{
    timer(core_context ctx) : ctx_(ctx)
    {
        get_cached_data(ctx, &data_);
        update();
    }
    timer(dataless_core_context ctx, timer_data& data)
        : ctx_(ctx), data_(&data)
    {
        update();
    }

    bool
    is_active() const
    {
        return data_->active;
    }

    void
    start(millisecond_count duration);

    void
    stop()
    {
        data_->active = false;
    }

    bool
    is_triggered()
    {
        return triggered_;
    }

 private:
    void
    update();

    dataless_core_context ctx_;
    timer_data* data_;
    bool triggered_;
};

namespace actions {

inline auto
start(timer& timer)
{
    return callback([&](millisecond_count duration) {
        timer.start(duration);
    });
}

inline auto
stop(timer& timer)
{
    return callback([&]() { timer.stop(); });
}

} // namespace actions

} // namespace alia

#endif
