#ifndef ALIA_ABI_KERNEL_ANIMATION_H
#define ALIA_ABI_KERNEL_ANIMATION_H

#include <alia/abi/base/color.h>
#include <alia/abi/bits.h>
#include <alia/abi/kernel/timing.h>
#include <alia/abi/prelude.h>

#include <limits.h>

ALIA_EXTERN_C_BEGIN

static inline alia_nanosecond_count
alia_animation_tick_count(alia_context* ctx)
{
    alia_timing_request_animation_refresh(ctx);
    return alia_timing_tick_count(ctx);
}

static inline alia_nanosecond_count
alia_animation_ticks_left(alia_context* ctx, alia_nanosecond_count end_time)
{
    alia_nanosecond_count ticks_remaining
        = end_time - alia_timing_tick_count(ctx);
    if (ticks_remaining > 0)
    {
        alia_timing_request_animation_refresh(ctx);
        return ticks_remaining;
    }
    return 0;
}

typedef uintptr_t alia_animation_id;

static inline alia_animation_id
alia_make_animation_id(alia_bitref bit)
{
    return (alia_animation_id) bit.storage
         ^ ((alia_animation_id) bit.offset
            << (sizeof(alia_animation_id) * CHAR_BIT - CHAR_BIT));
}

// FLARES

void
alia_animation_fire_flare(
    alia_context* ctx, alia_bitref bit, alia_nanosecond_count duration);

unsigned
alia_animation_process_flares(
    alia_context* ctx, alia_bitref bit, alia_nanosecond_count* tick_counts);

// TRANSITIONS

// unit cubic bezier with endpoints at (0, 0) and (1, 1)
typedef struct alia_unit_cubic_bezier
{
    float p1x;
    float p1y;
    float p2x;
    float p2y;
} alia_unit_cubic_bezier;

typedef alia_unit_cubic_bezier alia_animation_curve;

// built-in animation curves
extern alia_animation_curve const alia_default_curve;
extern alia_animation_curve const alia_linear_curve;
extern alia_animation_curve const alia_ease_in_curve;
extern alia_animation_curve const alia_ease_out_curve;
extern alia_animation_curve const alia_ease_in_out_curve;

// specification of the duration and path of an animation transition
typedef struct alia_animated_transition
{
    alia_animation_curve curve;
    alia_nanosecond_count duration;
} alia_animated_transition;

// Interpolate between `false_value` and `true_value` as `current_state`
// changes.
float
alia_transition_float(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    float true_value,
    float false_value);

alia_rgb
alia_transition_rgb(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    alia_rgb true_value,
    alia_rgb false_value);

alia_rgba
alia_transition_rgba(
    alia_context* ctx,
    const alia_animated_transition* transition,
    alia_bitref bits,
    bool current_state,
    alia_rgba true_value,
    alia_rgba false_value);

// CONTINUOUS VALUE SMOOTHING

// smoother for continuous values that change abruptly
typedef struct alia_float_smoother
{
    bool initialized;
    bool in_transition;
    alia_nanosecond_count duration;
    alia_nanosecond_count transition_end;
    float old_value;
    float new_value;
} alia_float_smoother;

// Snap `smoother` to `value` with no animation.
void
alia_float_smoother_reset(alia_float_smoother* smoother, float value);

// Advance `smoother` toward `target`.
// Sets `*out_animating` when animation is in progress.
// `out_animating` may be null.
float
alia_float_smoother_update(
    alia_float_smoother* smoother,
    float target,
    alia_animated_transition const* transition,
    alia_nanosecond_count now,
    bool* out_animating);

// Return a smoothed view of `target`.
// Requests an animation refresh when needed.
float
alia_smooth_float(
    alia_context* ctx,
    alia_float_smoother* smoother,
    float target,
    alia_animated_transition const* transition);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_KERNEL_ANIMATION_H
