#ifndef ALIA_TIMING_TICKS_HPP
#define ALIA_TIMING_TICKS_HPP

#include <alia/context/interface.hpp>
#include <alia/flow/actions.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

// Currently, alia's only sense of time is that of a monotonically increasing
// millisecond counter. It's understood to have an arbitrary start point and is
// allowed to wrap around, so 'unsigned' is considered sufficient.
typedef unsigned millisecond_count;

struct timing_subsystem
{
    millisecond_count tick_counter = 0;
};

// Request that the UI context refresh again quickly enough for smooth
// animation.
void
schedule_animation_refresh(dataless_context ctx);

// Get the value of the millisecond tick counter associated with the given
// UI context. This counter is updated every refresh pass, so it's consistent
// within a single frame.
// When this is called, it's assumed that something is currently animating, so
// it also requests a refresh.
millisecond_count
get_raw_animation_tick_count(dataless_context ctx);

// Same as above, but returns a signal rather than a raw integer.
value_signal<millisecond_count>
get_animation_tick_count(dataless_context ctx);

// Get the number of ticks remaining until the given end time.
// If the time has passed, this returns 0.
// This ensures that the UI context refreshes until the end time is reached.
millisecond_count
get_raw_animation_ticks_left(dataless_context ctx, millisecond_count end_tick);

struct animation_timer_state
{
    bool active = false;
    millisecond_count end_tick;
};

struct animation_timer
{
    animation_timer(context ctx) : ctx_(ctx)
    {
        get_cached_data(ctx, &state_);
        update();
    }
    animation_timer(dataless_context ctx, animation_timer_state& state)
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

    dataless_context ctx_;
    animation_timer_state* state_;
    millisecond_count ticks_left_;
};

namespace actions {

inline auto
start(animation_timer& timer)
{
    return callback(
        [&](millisecond_count duration) { timer.start(duration); });
}

} // namespace actions

} // namespace alia

#endif
