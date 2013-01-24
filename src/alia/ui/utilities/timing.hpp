#ifndef ALIA_UI_UTILITIES_TIMING_HPP
#define ALIA_UI_UTILITIES_TIMING_HPP

#include <alia/ui/internals.hpp>
#include <queue>

// This file provides various utilities for working with time in alia.

namespace alia {

// Request that the UI context refresh again quickly enough that an animation
// can be displayed smoothly.
void request_refresh(ui_context& ctx);

// Get the value of the millisecond tick counter associated with the given
// UI context. This counter is updated every refresh pass, so it's consistent
// within a single frame.
// When this is called, it's assumed that something is currently animating, so
// it also requests a refresh.
ui_time_type get_animation_tick_count(ui_context& ctx);

// Get the number of ticks remaining until the given end time.
// If the time has passed, this returns 0.
// This ensures that the UI context refreshes until the end time is reached.
ui_time_type get_animation_ticks_left(ui_context& ctx, ui_time_type end_time);

// Given a numeric value, smooth_value presents a view of that value where
// changes are smoothed out over time.
// (This function is supplied for both doubles and floats.)
float
smooth_value(ui_context& ctx, float x,
    animated_transition const& transition = default_transition);
double
smooth_value(ui_context& ctx, double x,
    animated_transition const& transition = default_transition);

// Same as above, but using accessors.
optional_input_accessor<float>
smooth_value(ui_context& ctx, getter<float> const& x,
    animated_transition const& transition = default_transition);
optional_input_accessor<double>
smooth_value(ui_context& ctx, getter<double> const& x,
    animated_transition const& transition = default_transition);

// Given an accessor to a value, this presents a delayed view of that value.
// Note that this function maintains a history of all value changes within
// the delay period, so using it on a frequently changing value with a long
// duration may consume a lot of memory.
template<class Value>
struct delayed_value_change
{
    ui_time_type time;
    optional<Value> value;
};
template<class Value>
struct delayed_value_data
{
    owned_id input_id;
    std::queue<delayed_value_change<Value> > history;
    optional<Value> current_output;
    local_identity identity;
    value_id_by_reference<local_id> id;
};
template<class Value>
custom_getter<Value>
delay_value(ui_context& ctx, accessor<Value> const& x, ui_time_type lag)
{
    delayed_value_data<Value>* data;
    get_cached_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        if (!data->input_id.matches(x.id()))
        {
            delayed_value_change<Value> change;
            change.time = get_animation_tick_count(ctx) + lag;
            change.value = is_gettable(x) ? optional<Value>(get(x)) : none;
            data->history.push(change);
            data->input_id.store(x.id());
        }
        while (!data->history.empty())
        {
            delayed_value_change<Value> const& oldest = data->history.front();
            if (get_animation_ticks_left(ctx, oldest.time) > 0)
                break;
            data->current_output = oldest.value;
            inc_version(data->identity);
            data->id = get_id(data->identity);
            data->history.pop();
        }
    }
    return make_custom_getter(&data->current_output, &data->id);
}

// This implements a one-shot timer that can be used to schedule time-dependent
// UI events.
//
// Any UI element that includes time-dependent behavior should use the
// functions provided here.  To schedule some timed behavior, call
// start_timer() and then on each pass through the control function, poll
// is_timer_done().  Once it returns true, the given time has elapsed.
// Note that is_timer_done() can only return true on a TIMER_EVENT (which is
// in the INPUT_CATEGORY).
//
void start_timer(ui_context& ctx, widget_id id, unsigned duration);
bool is_timer_done(ui_context& ctx, widget_id id);

// restart_timer() is similar to start_timer(), but it can only be invoked
// when handling a previous event (i.e, when is_timer_done() returns true).
// It adjusts the duration so that it's relative to when the event SHOULD HAVE
// occurred, rather than when it actually occurred.  This is useful for
// scheduling repeated events on a fixed frequency without drifting.
//
void restart_timer(ui_context& ctx, widget_id id, unsigned duration);

// Request that the UI refresh as quickly as possible and count the number of
// frames per second.
bool compute_fps(ui_context& ctx, int* fps);

}

#endif
