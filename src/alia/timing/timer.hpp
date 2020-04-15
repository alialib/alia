#ifndef ALIA_TIMING_TIMER_HPP
#define ALIA_TIMING_TIMER_HPP

#include <alia/context/interface.hpp>
#include <alia/flow/events.hpp>
#include <alia/system/internals.hpp>
#include <alia/timing/ticks.hpp>

// This file defines the interface to timer events in alia.

namespace alia {

struct timer_data : component_identity
{
    bool active = false;
    millisecond_count expected_trigger_time;
};

struct raw_timer
{
    raw_timer(context ctx) : ctx_(ctx)
    {
        get_cached_data(ctx, &data_);
        update();
    }
    raw_timer(dataless_context ctx, timer_data& data) : ctx_(ctx), data_(&data)
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

    dataless_context ctx_;
    timer_data* data_;
    bool triggered_;
};

} // namespace alia

#endif
