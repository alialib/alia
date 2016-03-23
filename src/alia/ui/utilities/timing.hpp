#ifndef ALIA_UI_UTILITIES_TIMING_HPP
#define ALIA_UI_UTILITIES_TIMING_HPP

#include <alia/ui/internals.hpp>
#include <queue>

// This file provides various utilities for working with time in alia.

namespace alia {

// Request that the UI context refresh again after the given time has elapsed.
void request_refresh(dataless_ui_context& ctx, ui_time_type duration);

// Request that the UI context refresh again quickly enough for smooth
// animation.
void request_animation_refresh(dataless_ui_context& ctx);

// Get the value of the millisecond tick counter associated with the given
// UI context. This counter is updated every refresh pass, so it's consistent
// within a single frame.
// When this is called, it's assumed that something is currently animating, so
// it also requests a refresh.
ui_time_type get_animation_tick_count(dataless_ui_context& ctx);

// Get the number of ticks remaining until the given end time.
// If the time has passed, this returns 0.
// This ensures that the UI context refreshes until the end time is reached.
ui_time_type
get_animation_ticks_left(dataless_ui_context& ctx, ui_time_type end_time);

// Generates a square wave.
// The return value alternates between true and false as time passes.
// The two duration parameters specify how long the signal remains at each
// value during a single cycle.
// If false_duration is omitted, true_duration is used in its place.
bool square_wave(ui_context& ctx, ui_time_type true_duration,
    ui_time_type false_duration = 0);

// A value_smoother is used to create smoothly changing views of values that
// actually change abruptly.
template<class Value>
struct value_smoother
{
    bool initialized, in_transition;
    ui_time_type duration, transition_end;
    Value old_value, new_value;
    value_smoother() : initialized(false) {}
};

// value_smoother requires the ability to interpolate the values it works with.
// If the value type supplies addition and multiplication by scalers, then it
// can be interpolated using the default implementation below. Another option
// is to simply implement a compatible interpolate function directly for the
// value type.

// interpolate(a, b, factor) yields a * (1 - factor) + b * factor
template<class Value>
Value interpolate(Value const& a, Value const& b, double factor)
{ return a * (1 - factor) + b * factor; }

static inline float interpolate(float a, float b, double factor)
{ return float(a * (1 - factor) + b * factor); }

static inline int interpolate(int a, int b, double factor)
{ return int(std::floor(a * (1 - factor) + b * factor + 0.5)); }

static inline uint8_t interpolate(uint8_t a, uint8_t b, double factor)
{ return uint8_t(std::floor(a * (1 - factor) + b * factor + 0.5)); }

// Interpolating a size_t is a little question, but it's useful when the size_t
// represents a count of something, so there's a special case for it to
// eliminate warnings.
static inline size_t interpolate(size_t a, size_t b, double factor)
{ return size_t(std::floor(double(a) * (1 - factor) + double(b) * factor + 0.5)); }

// reset_smoothing(smoother, value) causes the smoother to transition abruptly
// to the value specified.
template<class Value>
void reset_smoothing(value_smoother<Value>& smoother, Value const& value)
{
    smoother.in_transition = false;
    smoother.new_value = value;
    smoother.initialized = true;
}

// smooth_raw_value(ctx, smoother, x, transition) returns a smoothed view of x.
template<class Value>
Value
smooth_raw_value(
    dataless_ui_context& ctx, value_smoother<Value>& smoother, Value const& x,
    animated_transition const& transition = default_transition)
{
    if (!smoother.initialized)
        reset_smoothing(smoother, x);
    Value current_value = smoother.new_value;
    if (smoother.in_transition)
    {
        ui_time_type ticks_left =
            get_animation_ticks_left(ctx, smoother.transition_end);
        if (ticks_left > 0)
        {
            double fraction =
                eval_curve_at_x(transition.curve,
                    1. - double(ticks_left) / smoother.duration,
                    1. / smoother.duration);
            current_value =
                interpolate(smoother.old_value, smoother.new_value, fraction);
        }
        else
            smoother.in_transition = false;
    }
    if (is_refresh_pass(ctx) && x != smoother.new_value)
    {
        smoother.duration =
            // If we're just going back to the old value, go back in the same
            // amount of time it took to get here.
            smoother.in_transition && x == smoother.old_value ?
                (transition.duration -
                    get_animation_ticks_left(ctx, smoother.transition_end)) :
                transition.duration;
        smoother.transition_end =
            get_animation_tick_count(ctx) + smoother.duration;
        smoother.old_value = current_value;
        smoother.new_value = x;
        smoother.in_transition = true;
    }
    return current_value;
}

// If you don't need direct access to the smoother, this version will manage
// it for you.
template<class Value>
Value
smooth_raw_value(ui_context& ctx, Value const& x,
    animated_transition const& transition = default_transition)
{
    value_smoother<Value>* data;
    get_cached_data(ctx, &data);
    return smooth_raw_value(ctx, *data, x, transition);
}

// smooth_value is analagous to smooth_raw_value, but it deals with accessors
// instead of raw values.

template<class Value>
optional_input_accessor<Value>
smooth_value(
    dataless_ui_context& ctx, value_smoother<Value>& smoother,
    accessor<Value> const& x,
    animated_transition const& transition = default_transition)
{
    optional<Value> output;
    if (is_gettable(x))
        output = smooth_raw_value(ctx, smoother, get(x), transition);
    return optional_in(output);
}

template<class Value>
optional_input_accessor<Value>
smooth_value(ui_context& ctx, accessor<Value> const& x,
    animated_transition const& transition = default_transition)
{
    value_smoother<Value>* data;
    get_cached_data(ctx, &data);
    return smooth_value(ctx, *data, x, transition);
}

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
};
template<class Value>
custom_optional_getter<Value,value_id_by_reference<local_id> >
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
            data->history.pop();
        }
    }
    return make_custom_getter(&data->current_output, get_id(data->identity));
}

// This implements a one-shot timer that can be used to schedule time-dependent
// UI events.
void start_timer(dataless_ui_context& ctx, widget_id id, unsigned duration);
bool detect_timer_event(dataless_ui_context& ctx, widget_id id);

// restart_timer() is similar to start_timer(), but it can only be invoked
// when handling a previous event (i.e, when detect_timer_event() returns
// true).
// It adjusts the duration so that it's relative to when the event SHOULD HAVE
// occurred, rather than when it actually occurred. This allows repeating
// events to be scheduled on a fixed frequency without drifting.
void restart_timer(dataless_ui_context& ctx, widget_id id, unsigned duration);

// The timer object provides a more convenient interface to timer events.
// It is implemented on top of the above functions.
struct timer_data
{
    widget_identity id;
    bool active;
    timer_data() : active(false) {}
};
struct timer
{
    timer(ui_context& ctx, timer_data* data = 0);
    void start(unsigned duration);
    void stop();
    bool is_active() const { return data_->active; }
    bool triggered() const { return triggered_; }
 private:
    ui_context* ctx_;
    timer_data* data_;
    bool triggered_;
};

}

#endif
