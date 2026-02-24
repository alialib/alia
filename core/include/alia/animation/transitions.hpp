#pragma once

#include <cmath>
#include <cstdint>
#include <map>

#include <alia/animation/context.hpp>
#include <alia/animation/ids.hpp>
#include <alia/impl/base/bit_packing.hpp>
#include <alia/prelude.hpp>
#include <alia/ui/animation/unit_cubic_bezier.h>

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
    alia_nanosecond_count duration;
};
animated_transition const default_transition
    = {default_curve, milliseconds(250)};

struct transition_animation_data
{
    bool direction;
    alia_nanosecond_count transition_end;
};

using transition_animation_map
    = std::map<uintptr_t, transition_animation_data>;

template<class Context>
concept animated_transition_context
    = animation_context<Context> && requires(Context& ctx) {
          {
              get_animation_system(ctx).transitions
          } -> std::same_as<transition_animation_map&>;
      };

struct smoothing_bitfield : bitfield<2>
{
};

template<animated_transition_context Context>
void
init_transition(
    Context& ctx,
    bitref<smoothing_bitfield> bits,
    bool current_state,
    animated_transition const& transition)
{
    auto& animation
        = get_animation_system(ctx).transitions[make_animation_id(bits)];
    animation.direction = current_state;
    animation.transition_end
        = get_animation_tick_count(ctx) + transition.duration;
    bits = 0b01;
}

// TODO: Revisit lerping.

// lerp(a, b, factor) yields a * (1 - factor) + b * factor
template<class Value>
std::enable_if_t<!std::is_integral<Value>::value, Value>
lerp(Value const& a, Value const& b, float factor)
{
    return a * (1 - factor) + b * factor;
}
// Overload it for integers to add rounding (and eliminate warnings).
template<class Integer>
std::enable_if_t<std::is_integral<Integer>::value, Integer>
lerp(Integer a, Integer b, float factor)
{
    return Integer(std::round(a * (1 - factor) + b * factor));
}

template<class Value>
concept lerpable_value = requires(Value a, Value b, float factor) {
    { lerp(a, b, factor) } -> std::same_as<Value>;
};

template<lerpable_value Value, animated_transition_context Context>
Value
smooth_between_values(
    Context& ctx,
    bitref<smoothing_bitfield> bits,
    bool current_state,
    Value true_value,
    Value false_value,
    animated_transition const& transition = default_transition)
{
    switch (bits)
    {
        default:
        case 0b00:
            bits = current_state ? 0b11 : 0b10;
            return current_state ? true_value : false_value;
        case 0b01: {
            auto& animation = get_animation_system(ctx)
                                  .transitions[make_animation_id(bits)];
            alia_nanosecond_count ticks_left
                = get_animation_ticks_left(ctx, animation.transition_end);
            if (ticks_left > 0)
            {
                double fraction = eval_curve_at_x(
                    transition.curve,
                    1. - double(ticks_left) / transition.duration,
                    1. / transition.duration);
                Value current_value = lerp(
                    animation.direction ? false_value : true_value,
                    animation.direction ? true_value : false_value,
                    fraction);
                if (current_state != animation.direction)
                {
                    // Go back in the same amount of time it took to get here.
                    // In order to do this, we have to solve for the time it
                    // will take to get back here.
                    animation.transition_end
                        = get_animation_tick_count(ctx)
                        + alia_nanosecond_count(
                              transition.duration
                              * (1
                                 - eval_curve_at_x(
                                     unit_cubic_bezier{
                                         1 - transition.curve.p1x,
                                         1 - transition.curve.p1y,
                                         1 - transition.curve.p2x,
                                         1 - transition.curve.p2y},
                                     1 - fraction,
                                     1. / transition.duration)));
                    animation.direction = current_state;
                }
                return current_value;
            }
            else
            {
                auto end_state = animation.direction;
                get_animation_system(ctx).transitions.erase(
                    make_animation_id(bits));
                bits = end_state ? 0b11 : 0b10;
                return end_state ? true_value : false_value;
            }
        }
        case 0b10:
            if (current_state)
                init_transition(ctx, bits, true, transition);
            return false_value;
        case 0b11:
            if (!current_state)
                init_transition(ctx, bits, false, transition);
            return true_value;
    }
}

} // namespace alia
