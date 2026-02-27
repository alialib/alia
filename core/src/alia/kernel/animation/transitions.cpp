#include <alia/abi/kernel/animation.h>

#include <cmath>
#include <type_traits>

#include <alia/abi/base/color.h>
#include <alia/impl/kernel/animation.hpp>
#include <alia/kernel/animation/unit_cubic_bezier.h>
#include <alia/system/object.hpp>

namespace alia { namespace impl {

void
start_transition(
    alia_context* ctx,
    alia_bitref bits,
    bool current_state,
    alia_animated_transition const& transition)
{
    auto& animation
        = ctx->system->animation.transitions[alia_make_animation_id(bits)];
    animation.direction = current_state;
    animation.transition_end
        = alia_animation_tick_count(ctx) + transition.duration;
    alia_bitref_write_pair(bits, 0b01);
}

float
update_transition(
    alia_context* ctx,
    alia_bitref bits,
    bool current_state,
    alia_animated_transition const& transition)
{
    auto& animation
        = ctx->system->animation.transitions[alia_make_animation_id(bits)];
    alia_nanosecond_count ticks_left
        = alia_animation_ticks_left(ctx, animation.transition_end);
    if (ticks_left > 0)
    {
        double fraction = eval_curve_at_x(
            transition.curve,
            1. - double(ticks_left) / transition.duration,
            1. / transition.duration);
        if (current_state != animation.direction)
        {
            // Go back in the same amount of time it took to get here.
            // In order to do this, we have to solve for the time it
            // will take to get back here.
            animation.transition_end
                = alia_animation_tick_count(ctx)
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
        return fraction;
    }
    else
    {
        auto end_state = animation.direction;
        ctx->system->animation.transitions.erase(alia_make_animation_id(bits));
        alia_bitref_write_pair(bits, end_state ? 0b11 : 0b10);
        return end_state ? 1.f : 0.f;
    }
}

}} // namespace alia::impl

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_animation_curve const alia_default_curve = {0.25, 0.1, 0.25, 1};
alia_animation_curve const alia_linear_curve = {0, 0, 1, 1};
alia_animation_curve const alia_ease_in_curve = {0.42, 0, 1, 1};
alia_animation_curve const alia_ease_out_curve = {0, 0, 0.58, 1};
alia_animation_curve const alia_ease_in_out_curve = {0.42, 0, 0.58, 1};

// TODO: Move this somewhere in base/.
float
lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float
alia_smooth_float(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    float true_value,
    float false_value)
{
    return impl::smooth(
        ctx, *transition, lerp, bits, current_state, true_value, false_value);
}

alia_rgb
alia_smooth_rgb(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    alia_rgb true_value,
    alia_rgb false_value)
{
    return impl::smooth(
        ctx,
        *transition,
        alia_lerp_rgb_via_oklch,
        bits,
        current_state,
        true_value,
        false_value);
}

alia_rgba
alia_smooth_rgba(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    alia_rgba true_value,
    alia_rgba false_value)
{
    return impl::smooth(
        ctx,
        *transition,
        alia_lerp_rgba_via_oklch,
        bits,
        current_state,
        true_value,
        false_value);
}

ALIA_EXTERN_C_END
