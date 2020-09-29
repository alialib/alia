#ifndef ALIA_TIMING_WAVES_HPP
#define ALIA_TIMING_WAVES_HPP

#include <alia/context/interface.hpp>
#include <alia/signals/adaptors.hpp>
#include <alia/signals/basic.hpp>
#include <alia/timing/ticks.hpp>

namespace alia {

namespace detail {

value_signal<bool>
square_wave(
    context ctx,
    readable<millisecond_count> true_duration,
    readable<millisecond_count> false_duration);

}

// Generates a square wave.
//
// The returned signal alternates between true and false as time passes.
//
// The two duration parameters specify how long (in milliseconds) the signal
// remains at each value during a single cycle.
//
// Both durations can be either signals or raw values.
//
// If :false_duration doesn't have a value (or is omitted), :true_duration is
// used in its place.
//
// If :true_duration doesn't have a value, the square wave will stop
// alternating.
//
template<
    class TrueDuration,
    class FalseDuration = empty_signal<millisecond_count>>
value_signal<bool>
square_wave(
    context ctx,
    TrueDuration true_duration,
    FalseDuration false_duration = empty<millisecond_count>())
{
    return detail::square_wave(
        ctx,
        signal_cast<millisecond_count>(signalize(true_duration)),
        signal_cast<millisecond_count>(signalize(false_duration)));
}

} // namespace alia

#endif
