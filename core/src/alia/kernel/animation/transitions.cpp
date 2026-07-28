#include <alia/abi/kernel/animation.h>

#include <cmath>
#include <type_traits>

#include <alia/abi/base/color.h>
#include <alia/impl/events.hpp>
#include <alia/impl/kernel/animation.hpp>
#include <alia/kernel/animation/unit_cubic_bezier.h>
#include <alia/ui/system/object.h>

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
    if (current_state != animation.direction)
    {
        float fraction = eval_curve_at_x(
            transition.curve,
            1. - float(ticks_left) / float(transition.duration),
            0.00001);
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
                         0.00001)));
        animation.direction = current_state;
        return current_state ? 1.f - fraction : fraction;
    }
    else if (ticks_left > 0)
    {
        float fraction = eval_curve_at_x(
            transition.curve,
            1. - float(ticks_left) / float(transition.duration),
            0.00001);
        return current_state ? fraction : 1.f - fraction;
    }
    else
    {
        ctx->system->animation.transitions.erase(alia_make_animation_id(bits));
        alia_bitref_write_pair(bits, current_state ? 0b11 : 0b10);
        return current_state ? 1.f : 0.f;
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

float
alia_transition_float(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    float true_value,
    float false_value)
{
    return impl::transition_between(
        ctx,
        *transition,
        alia_lerp,
        bits,
        current_state,
        true_value,
        false_value);
}

alia_rgb
alia_transition_rgb(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    alia_rgb true_value,
    alia_rgb false_value)
{
    return impl::transition_between(
        ctx,
        *transition,
        alia_lerp_rgb_raw,
        bits,
        current_state,
        true_value,
        false_value);
}

alia_rgba
alia_transition_rgba(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    alia_rgba true_value,
    alia_rgba false_value)
{
    return impl::transition_between(
        ctx,
        *transition,
        alia_lerp_rgba_raw,
        bits,
        current_state,
        true_value,
        false_value);
}

void
alia_float_smoother_reset(alia_float_smoother* smoother, float value)
{
    ALIA_ASSERT(smoother);
    smoother->initialized = true;
    smoother->in_transition = false;
    smoother->duration = 0;
    smoother->transition_end = 0;
    smoother->old_value = value;
    smoother->new_value = value;
}

float
alia_float_smoother_update(
    alia_float_smoother* smoother,
    float target,
    alia_animated_transition const* transition,
    alia_nanosecond_count now,
    bool* out_animating)
{
    ALIA_ASSERT(smoother);
    ALIA_ASSERT(transition);

    bool animating = false;

    if (!smoother->initialized)
        alia_float_smoother_reset(smoother, target);

    float current_value = smoother->new_value;
    if (smoother->in_transition)
    {
        alia_nanosecond_count const ticks_left
            = smoother->transition_end - now;
        if (ticks_left > 0 && smoother->transition_end > now)
        {
            float const fraction = eval_curve_at_x(
                transition->curve,
                1.f - float(ticks_left) / float(smoother->duration),
                0.00001f);
            current_value = alia_lerp(
                smoother->old_value, smoother->new_value, fraction);
            animating = true;
        }
        else
        {
            smoother->in_transition = false;
            current_value = smoother->new_value;
        }
    }

    if (target != smoother->new_value)
    {
        // If reversing to the previous endpoint, finish in the time it took
        // to get here.
        if (smoother->in_transition && target == smoother->old_value
            && smoother->transition_end > now)
        {
            smoother->duration
                = smoother->duration - (smoother->transition_end - now);
            if (smoother->duration < 1)
                smoother->duration = 1;
        }
        else
        {
            smoother->duration = transition->duration;
        }
        smoother->transition_end = now + smoother->duration;
        smoother->old_value = current_value;
        smoother->new_value = target;
        smoother->in_transition = true;
        animating = true;
    }

    if (out_animating)
        *out_animating = animating;
    return current_value;
}

float
alia_smooth_float(
    alia_context* ctx,
    alia_float_smoother* smoother,
    float target,
    alia_animated_transition const* transition)
{
    ALIA_ASSERT(ctx);
    bool animating = false;
    float const value = alia_float_smoother_update(
        smoother, target, transition, alia_timing_tick_count(ctx), &animating);
    if (animating)
        alia_timing_request_animation_refresh(ctx);
    return value;
}

ALIA_EXTERN_C_END
