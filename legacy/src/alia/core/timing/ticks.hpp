#ifndef ALIA_CORE_TIMING_TICKS_HPP
#define ALIA_CORE_TIMING_TICKS_HPP

#include <alia/core/actions/basic.hpp>
#include <alia/core/context/interface.hpp>
#include <alia/core/flow/data_graph.hpp>

namespace alia {

// Currently, alia's only sense of time is that of a monotonically increasing
// millisecond counter. It's understood to have an arbitrary start point and is
// allowed to wrap around, so 'unsigned' is considered sufficient.
typedef unsigned millisecond_count;

struct timing_subsystem
{
    millisecond_count tick_counter = 0;
};

struct animation_timer_state
{
    // TODO: Combine these.
    bool active = false;
    millisecond_count end_tick;
};

struct animation_timer
{
    animation_timer(core_context ctx) : ctx_(ctx)
    {
        get_cached_data(ctx, &state_);
        update();
    }
    animation_timer(dataless_core_context ctx, animation_timer_state& state)
        : ctx_(ctx), state_(&state)
    {
        update();
    }
    bool
    is_active() const
    {
        return state_->active;
    }
    millisecond_count
    ticks_left() const
    {
        return ticks_left_;
    }
    void
    start(millisecond_count duration)
    {
        state_->active = true;
        state_->end_tick = get_raw_animation_tick_count(ctx_) + duration;
    }

 private:
    void
    update()
    {
        if (state_->active)
        {
            ticks_left_ = get_raw_animation_ticks_left(ctx_, state_->end_tick);
            if (ticks_left_ == 0)
                state_->active = false;
        }
        else
        {
            ticks_left_ = 0;
        }
    }

    dataless_core_context ctx_;
    animation_timer_state* state_;
    millisecond_count ticks_left_;
};

namespace actions {

inline auto
start(animation_timer& timer)
{
    return callback([&](millisecond_count duration) {
        timer.start(duration);
    });
}

} // namespace actions

} // namespace alia

#endif
