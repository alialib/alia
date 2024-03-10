#ifndef ALIA_CORE_TIMING_SMOOTHING_HPP
#define ALIA_CORE_TIMING_SMOOTHING_HPP

#include <alia/core/context/interface.hpp>
#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/signals/adaptors.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/timing/cubic_bezier.hpp>
#include <alia/core/timing/ticks.hpp>

#include <cmath>

namespace alia {

// The following are interpolation curves that can be used for animations.
typedef unit_cubic_bezier animation_curve;
animation_curve const default_curve = {0.25, 0.1, 0.25, 1};
animation_curve const linear_curve = {0, 0, 1, 1};
animation_curve const ease_in_curve = {0.42, 0, 1, 1};
animation_curve const ease_out_curve = {0, 0, 0.58, 1};
animation_curve const ease_in_out_curve = {0.42, 0, 0.58, 1};

// animated_transition specifies an animated transition from one state to
// another, defined by a duration and a curve to follow.
struct animated_transition
{
    animation_curve curve;
    millisecond_count duration;
};
animated_transition const default_transition = {default_curve, 250};

// A value_smoother is used to create smoothly changing views of values that
// actually change abruptly.
template<class Value>
struct value_smoother
{
    bool initialized = false, in_transition;
    millisecond_count duration, transition_end;
    Value old_value, new_value;
};

// value_smoother requires the ability to interpolate the values it works with.
// If the value type supplies addition and multiplication by scalers, then it
// can be interpolated using the default implementation below. Another option
// is to simply implement a compatible interpolate function directly for the
// value type.

// interpolate(a, b, factor) yields a * (1 - factor) + b * factor
template<class Value>
std::enable_if_t<!std::is_integral<Value>::value, Value>
interpolate(Value const& a, Value const& b, double factor)
{
    return a * (1 - factor) + b * factor;
}
// Overload it for floats to eliminate warnings about conversions.
inline float
interpolate(float a, float b, double factor)
{
    return float(a * (1 - factor) + b * factor);
}
// Overload it for integers to add rounding (and eliminate warnings).
template<class Integer>
std::enable_if_t<std::is_integral<Integer>::value, Integer>
interpolate(Integer a, Integer b, double factor)
{
    return Integer(std::round(a * (1 - factor) + b * factor));
}

// reset_smoothing(smoother, value) causes the smoother to transition abruptly
// to the value specified.
template<class Value>
void
reset_smoothing(value_smoother<Value>& smoother, Value const& value)
{
    smoother.in_transition = false;
    smoother.new_value = value;
    smoother.initialized = true;
}

// smooth_raw(ctx, smoother, x, transition) returns a smoothed view of x.
template<class Value>
Value
smooth_raw(
    dataless_core_context ctx,
    value_smoother<Value>& smoother,
    Value const& x,
    animated_transition const& transition = default_transition)
{
    if (!smoother.initialized)
        reset_smoothing(smoother, x);
    Value current_value = smoother.new_value;
    if (smoother.in_transition)
    {
        millisecond_count ticks_left
            = get_raw_animation_ticks_left(ctx, smoother.transition_end);
        if (ticks_left > 0)
        {
            double fraction = eval_curve_at_x(
                transition.curve,
                1. - double(ticks_left) / smoother.duration,
                1. / smoother.duration);
            current_value = interpolate(
                smoother.old_value, smoother.new_value, fraction);
        }
        else
            smoother.in_transition = false;
    }
    if (is_refresh_event(ctx) && x != smoother.new_value)
    {
        smoother.duration =
            // If we're just going back to the old value, go back in the same
            // amount of time it took to get here.
            smoother.in_transition && x == smoother.old_value
                ? (transition.duration
                   - get_raw_animation_ticks_left(
                       ctx, smoother.transition_end))
                : transition.duration;
        smoother.transition_end
            = get_raw_animation_tick_count(ctx) + smoother.duration;
        smoother.old_value = current_value;
        smoother.new_value = x;
        smoother.in_transition = true;
    }
    return current_value;
}

// smooth is analogous to smooth_raw, but it deals with signals instead of raw
// values.

template<class Wrapped>
struct smoothed_signal : signal_wrapper<
                             smoothed_signal<Wrapped>,
                             Wrapped,
                             typename Wrapped::value_type,
                             typename signal_capabilities_intersection<
                                 typename Wrapped::capabilities,
                                 readable_duplex_signal>::type>
{
    smoothed_signal(
        Wrapped wrapped,
        value_smoother<typename Wrapped::value_type>& smoother,
        typename Wrapped::value_type smoothed_value)
        : smoothed_signal::signal_wrapper(wrapped),
          smoother_(smoother),
          smoothed_value_(smoothed_value)
    {
    }
    id_interface const&
    value_id() const override
    {
        if (this->wrapped_.has_value())
        {
            id_ = make_id(smoothed_value_);
            return id_;
        }
        else
            return null_id;
    }
    typename Wrapped::value_type const&
    read() const override
    {
        return smoothed_value_;
    }
    id_interface const&
    write(typename Wrapped::value_type value) const override
    {
        this->wrapped_.write(value);
        reset_smoothing(smoother_, value);
        return null_id;
    }

 private:
    value_smoother<typename Wrapped::value_type>& smoother_;
    typename Wrapped::value_type smoothed_value_;
    mutable simple_id<typename Wrapped::value_type> id_;
};
template<class Wrapped>
smoothed_signal<Wrapped>
make_smoothed_signal(
    Wrapped wrapped,
    value_smoother<typename Wrapped::value_type>& smoother,
    typename Wrapped::value_type smoothed_value)
{
    return smoothed_signal<Wrapped>(wrapped, smoother, smoothed_value);
}

template<class Value, class Signal>
auto
smooth(
    dataless_core_context ctx,
    value_smoother<Value>& smoother,
    Signal x,
    animated_transition const& transition = default_transition)
{
    Value output = Value();
    if (signal_has_value(x))
        output = smooth_raw(ctx, smoother, read_signal(x), transition);
    return make_smoothed_signal(x, smoother, output);
}

template<class Signal>
auto
smooth(
    core_context ctx,
    Signal x,
    animated_transition const& transition = default_transition)
{
    value_smoother<typename Signal::value_type>* data;
    get_cached_data(ctx, &data);
    return smooth(ctx, *data, x, transition);
}

} // namespace alia

#endif
