#include "alia/ui/system/input_constants.hpp"
#include <alia/ui/utilities/click_flares.hpp>

#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>

#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>

namespace alia {

void
fire_click_flare(
    dataless_ui_context ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    mouse_button button)
{
    if (get_system(ctx).tick_count - get_click_start_time(ctx, button) < 200)
    {
        fire_flare(ctx, ALIA_BITREF(bits, click_flare), click_flare_duration);
    }
    else
    {
        fire_flare(
            ctx,
            ALIA_BITREF(bits, press_and_hold_flare),
            (std::min)(
                click_accumulation_time,
                get_system(ctx).tick_count - get_click_start_time(ctx, button))
                / 2);
    }
}

void
render_click_flares(
    dataless_ui_context ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    interaction_status state,
    layout_vector position,
    rgb8 color,
    float radius)
{
    process_flares(
        ctx,
        ALIA_BITREF(bits, click_flare),
        [&](millisecond_count ticks_left) {
            float intensity = float(eval_curve_at_x(
                animation_curve{0.2, 0, 1, 1},
                float(ticks_left) / click_flare_duration,
                0.001));
            float radius_scale_factor = float(eval_curve_at_x(
                animation_curve{0, 0, 0.9, 1},
                1
                    - (std::fmax)(
                        0.0f,
                        (float(ticks_left) - 300)
                            / (click_flare_duration - 300)),
                0.001));
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor(SkColorSetARGB(
                uint8_t(intensity * 0x60), color.r, color.g, color.b));
            auto& event = cast_event<render_event>(ctx);
            SkCanvas& canvas = *event.canvas;
            canvas.drawPath(
                SkPath::Circle(
                    position[0], position[1], radius * radius_scale_factor),
                paint);
        });

    process_flares(
        ctx,
        ALIA_BITREF(bits, press_and_hold_flare),
        [&](millisecond_count ticks_left) {
            float intensity = float(ticks_left) / 200;
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor(SkColorSetARGB(
                uint8_t(intensity * 0x60), color.r, color.g, color.b));
            auto& event = cast_event<render_event>(ctx);
            SkCanvas& canvas = *event.canvas;
            canvas.drawPath(
                SkPath::Circle(position[0], position[1], radius), paint);
        });

    if (is_active(state))
    {
        millisecond_count click_duration = get_click_duration(
            ctx, mouse_button::LEFT, click_accumulation_time);
        float intensity = float(click_duration) / click_accumulation_time;
        float radius_scale_factor
            = float(click_duration) / click_accumulation_time;
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(
            uint8_t(intensity * 0x60), color.r, color.g, color.b));
        auto& event = cast_event<render_event>(ctx);
        SkCanvas& canvas = *event.canvas;
        canvas.drawPath(
            SkPath::Circle(
                position[0], position[1], radius_scale_factor * radius),
            paint);
    }
}

} // namespace alia
