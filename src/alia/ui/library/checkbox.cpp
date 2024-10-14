#include <alia/ui/library/checkbox.hpp>

#include <alia/ui/color.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/utilities/animation.hpp>
#include <alia/ui/utilities/click_flares.hpp>
#include <alia/ui/utilities/skia.hpp>

#include <include/core/SkBlurTypes.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>

namespace alia {

struct checkbox_bit_layout
{
    click_flare_bit_layout click_flare;
    smoothing_bit_field state_smoothing;
};

struct checkbox_data
{
    bitpack<checkbox_bit_layout> bits;
    keyboard_click_state keyboard_click_state_;
    layout_leaf layout_node;
};

void
do_checkbox(ui_context ctx, duplex<bool> checked, layout const& layout_spec)
{
    checkbox_data* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    auto const id = data_ptr;

    bool const is_disabled = !signal_ready_to_write(checked);

    alia_untracked_switch(get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(make_layout_vector(48, 48), 0, 0),
                LEFT | BASELINE_Y | PADDED);

            add_layout_node(
                get<ui_traversal_tag>(ctx).layout, &data.layout_node);

            break;

        case REGION_CATEGORY:
            do_box_region(
                ctx, id, box<2, double>(data.layout_node.assignment().region));
            break;

        case INPUT_CATEGORY:
            if (is_disabled)
                break;

            add_to_focus_order(ctx, id);

            if (detect_click(ctx, id, mouse_button::LEFT)
                || detect_keyboard_click(ctx, data.keyboard_click_state_, id))
            {
                // TODO: Handle timing properly for keyboard clicks.
                fire_click_flare(
                    ctx, ALIA_NESTED_BITPACK(data.bits, click_flare));
                if (signal_has_value(checked)
                    && signal_ready_to_write(checked))
                {
                    write_signal(checked, !read_signal(checked));
                }
                abort_traversal(ctx);
            }

            break;

        case RENDER_CATEGORY: {
            auto& event = cast_event<render_event>(ctx);

            SkCanvas& canvas = *event.canvas;

            auto const& region = data.layout_node.assignment().region;

            SkRect rect = as_skrect(region);

            if (event.canvas->quickReject(rect))
                break;

            if (is_disabled)
            {
                float const padding = 11.f;
                auto checkbox_rect = remove_border(
                    region,
                    box_border_width<float>{
                        padding, padding, padding, padding});

                if (condition_is_true(checked))
                {
                    {
                        SkPaint paint;
                        paint.setAntiAlias(true);
                        paint.setStyle(SkPaint::kStrokeAndFill_Style);
                        paint.setColor(SkColorSetARGB(0xff, 0x60, 0x60, 0x66));
                        paint.setStrokeWidth(4);
                        canvas.drawPath(
                            SkPath::RRect(as_skrect(checkbox_rect), 2, 2),
                            paint);
                    }
                    {
                        SkPaint paint;
                        paint.setAntiAlias(true);
                        paint.setStyle(SkPaint::kStroke_Style);
                        paint.setColor(SkColorSetARGB(0xff, 0x20, 0x20, 0x20));
                        paint.setStrokeWidth(4);
                        paint.setStrokeCap(SkPaint::kSquare_Cap);
                        SkPath path;
                        path.incReserve(3);
                        SkPoint p0;
                        p0.fX = checkbox_rect.corner[0]
                              + checkbox_rect.size[0] * 0.2f;
                        p0.fY = checkbox_rect.corner[1]
                              + checkbox_rect.size[1] * 0.5f;
                        path.moveTo(p0);
                        SkPoint p1;
                        p1.fX = checkbox_rect.corner[0]
                              + checkbox_rect.size[0] * 0.4f;
                        p1.fY = checkbox_rect.corner[1]
                              + checkbox_rect.size[1] * 0.7f;
                        path.lineTo(p1);
                        SkPoint p2;
                        p2.fX = checkbox_rect.corner[0]
                              + checkbox_rect.size[0] * 0.8f;
                        p2.fY = checkbox_rect.corner[1]
                              + checkbox_rect.size[1] * 0.3f;
                        path.lineTo(p2);
                        canvas.drawPath(path, paint);
                    }
                }
                else
                {
                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setStyle(SkPaint::kStroke_Style);
                    paint.setColor(SkColorSetARGB(0xff, 0x60, 0x60, 0x66));
                    paint.setStrokeWidth(4);
                    canvas.drawPath(
                        SkPath::RRect(as_skrect(checkbox_rect), 2, 2), paint);
                }

                break;
            }

            auto center = get_center(region);

            float smoothed_state = smooth_between_values(
                ctx,
                ALIA_BITREF(data.bits, state_smoothing),
                condition_is_true(checked),
                1.f,
                0.f,
                animated_transition{default_curve, 200});

            rgb8 color = interpolate(
                rgb8(0xa0, 0xa0, 0xa0),
                rgb8(0x90, 0xc0, 0xff),
                smoothed_state);

            uint8_t highlight = 0;
            if (is_click_in_progress(ctx, id, mouse_button::LEFT)
                || is_pressed(data.keyboard_click_state_))
            {
                // highlight = 0x40;
                highlight = 0x20;
            }
            else if (is_click_possible(ctx, id))
            {
                highlight = 0x20;
            }
            if (highlight != 0)
            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setColor(
                    SkColorSetARGB(highlight, color.r, color.g, color.b));
                canvas.drawPath(
                    SkPath::Circle(center[0], center[1], 32.f), paint);
            }

            float const padding = 11.f;
            auto checkbox_rect = remove_border(
                region,
                box_border_width<float>{padding, padding, padding, padding});

            auto state = get_widget_state(
                ctx,
                id,
                (is_disabled ? WIDGET_DISABLED : NO_FLAGS)
                    | (is_pressed(data.keyboard_click_state_)
                           ? WIDGET_DEPRESSED
                           : NO_FLAGS));

            if (condition_is_true(checked))
            {
                {
                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setStyle(SkPaint::kStrokeAndFill_Style);
                    paint.setColor(SkColorSetARGB(0xff, 0x90, 0xc0, 0xff));
                    paint.setStrokeWidth(4);
                    canvas.drawPath(
                        SkPath::RRect(as_skrect(checkbox_rect), 2, 2), paint);
                }
                {
                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setStyle(SkPaint::kStroke_Style);
                    paint.setColor(SkColorSetARGB(0xff, 0x20, 0x20, 0x20));
                    paint.setStrokeWidth(4);
                    paint.setStrokeCap(SkPaint::kSquare_Cap);
                    SkPath path;
                    path.incReserve(3);
                    SkPoint p0;
                    p0.fX = checkbox_rect.corner[0]
                          + checkbox_rect.size[0] * 0.2f;
                    p0.fY = checkbox_rect.corner[1]
                          + checkbox_rect.size[1] * 0.5f;
                    path.moveTo(p0);
                    SkPoint p1;
                    p1.fX = checkbox_rect.corner[0]
                          + checkbox_rect.size[0] * 0.4f;
                    p1.fY = checkbox_rect.corner[1]
                          + checkbox_rect.size[1] * 0.7f;
                    path.lineTo(p1);
                    SkPoint p2;
                    p2.fX = checkbox_rect.corner[0]
                          + checkbox_rect.size[0] * 0.8f;
                    p2.fY = checkbox_rect.corner[1]
                          + checkbox_rect.size[1] * 0.3f;
                    path.lineTo(p2);
                    canvas.drawPath(path, paint);
                }
            }
            else
            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setColor(SkColorSetARGB(0xff, 0xc0, 0xc0, 0xc0));
                paint.setStrokeWidth(4);
                canvas.drawPath(
                    SkPath::RRect(as_skrect(checkbox_rect), 2, 2), paint);
            }

            render_click_flares(
                ctx,
                ALIA_NESTED_BITPACK(data.bits, click_flare),
                state,
                center,
                color);
        }
    }
    alia_end
}

} // namespace alia
