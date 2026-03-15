#include <alia/ui/animation.h>

#include <alia/abi/kernel/animation.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/animation/unit_cubic_bezier.h>
#include <alia/ui/drawing.h>

#include <algorithm>

namespace alia {

void
fire_click_flare(
    alia_context* ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    alia_button_t button)
{
    if (alia_animation_tick_count(ctx)
            - alia_input_click_start_time(ctx, button)
        < milliseconds(200))
    {
        alia_animation_fire_flare(
            ctx, ALIA_BITREF(bits, click_flare), click_flare_duration);
    }
    else
    {
        alia_animation_fire_flare(
            ctx,
            ALIA_BITREF(bits, press_and_hold_flare),
            (std::min) (click_accumulation_time,
                        alia_animation_tick_count(ctx)
                            - alia_input_click_start_time(ctx, button))
                / 2);
    }
}

void
render_click_flares(
    alia_context* ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    alia_interaction_status_t state,
    alia_vec2f position,
    alia_srgb8 color,
    float radius)
{
    // TODO: Don't hardcode the maximum number of flares.
    alia_nanosecond_count flare_ticks_left[8];
    {
        unsigned flare_count = alia_animation_process_flares(
            ctx, ALIA_BITREF(bits, click_flare), flare_ticks_left);
        for (unsigned i = 0; i < flare_count; ++i)
        {
            alia_nanosecond_count ticks_left = flare_ticks_left[i];
            float intensity = float(eval_curve_at_x(
                alia_animation_curve{0.2, 0, 1, 1},
                float(ticks_left) / click_flare_duration,
                0.00001));
            float radius_scale_factor = float(eval_curve_at_x(
                alia_animation_curve{0, 0, 0.9, 1},
                1
                    - (std::max) (0.0f,
                                  (float(ticks_left - milliseconds(300))
                                   / float(
                                       click_flare_duration
                                       - milliseconds(300)))),
                0.00001));
            alia_draw_circle(
                ctx,
                ctx->geometry->z_base + 4, // TODO: ALIA_Z_INDEX_EFFECTS?
                position,
                radius * radius_scale_factor,
                alia_srgba8_from_srgb8_alpha(
                    color, uint8_t(intensity * 0x66)));
        }
    }

    {
        alia_nanosecond_count flare_ticks_left[8];
        unsigned flare_count = alia_animation_process_flares(
            ctx, ALIA_BITREF(bits, press_and_hold_flare), flare_ticks_left);
        for (unsigned i = 0; i < flare_count; ++i)
        {
            alia_nanosecond_count ticks_left = flare_ticks_left[i];
            float intensity = float(ticks_left) / float(milliseconds(200));
            alia_draw_circle(
                ctx,
                ctx->geometry->z_base + 4, // TODO: ALIA_Z_INDEX_EFFECTS?
                position,
                radius,
                alia_srgba8_from_srgb8_alpha(
                    color, uint8_t(intensity * 0x66)));
        }
    }

    if (state & ALIA_INTERACTION_STATUS_ACTIVE)
    {
        alia_nanosecond_count const capped_duration
            = click_accumulation_time
            - alia_animation_ticks_left(
                  ctx,
                  alia_input_click_start_time(ctx, ALIA_BUTTON_LEFT)
                      + click_accumulation_time);
        float scale_factor
            = float(capped_duration) / float(click_accumulation_time);
        scale_factor = eval_curve_at_x(
            alia_animation_curve{0.2, 0, 1, 1}, scale_factor, 0.00001);
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 4, // TODO: ALIA_Z_INDEX_EFFECTS?
            position,
            scale_factor * radius,
            alia_srgba8_from_srgb8_alpha(color, uint8_t(scale_factor * 0x66)));
    }
}

} // namespace alia
