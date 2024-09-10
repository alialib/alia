#include <alia/ui/library/radio_button.hpp>

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

struct radio_button_data
{
    unsigned bits;

    static constexpr unsigned click_flare_bits = 0;
    static constexpr unsigned state_smoothing_bits
        = click_flare_bits + click_flare_bit_count;
    static constexpr unsigned total_bits_used
        = state_smoothing_bits + bits_required_for_smoothing;
    static_assert(total_bits_used <= 32);

    keyboard_click_state keyboard_click_state_;
    layout_leaf layout_node;
};

void
do_radio_button(
    ui_context ctx,
    duplex<bool> selected,
    layout const& layout_spec,
    widget_id id)
{
    radio_button_data* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    if (!id)
        id = data_ptr;

    auto const is_disabled = !signal_ready_to_write(selected);

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
                return;

            add_to_focus_order(ctx, id);

            if (detect_click(ctx, id, mouse_button::LEFT)
                || detect_keyboard_click(ctx, data.keyboard_click_state_, id))
            {
                // TODO: It's not necessarily the left mouse button.
                fire_click_flare(
                    ctx, mouse_button::LEFT, data.bits, data.click_flare_bits);
                if (signal_ready_to_write(selected))
                    write_signal(selected, true);
                abort_traversal(ctx);
            }

            break;

        case RENDER_CATEGORY: {
            auto& event = cast_event<render_event>(ctx);

            SkCanvas& canvas = *event.canvas;

            auto const& region = data.layout_node.assignment().region;

            SkRect rect;
            rect.fLeft = SkScalar(region.corner[0]);
            rect.fTop = SkScalar(region.corner[1]);
            rect.fRight = SkScalar(region.corner[0] + region.size[0]);
            rect.fBottom = SkScalar(region.corner[1] + region.size[1]);

            if (event.canvas->quickReject(rect))
                break;

            auto center = get_center(region);

            if (is_disabled)
            {
                {
                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setStyle(SkPaint::kStroke_Style);
                    paint.setColor(SkColorSetARGB(0xff, 0x60, 0x60, 0x66));
                    paint.setStrokeWidth(3);
                    canvas.drawPath(
                        SkPath::Circle(center[0], center[1], 15.f), paint);
                }

                if (condition_is_true(selected))
                {
                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setColor(SkColorSetARGB(0xff, 0x80, 0x80, 0x88));
                    paint.setStrokeWidth(3);
                    canvas.drawPath(
                        SkPath::Circle(center[0], center[1], 10), paint);
                }

                break;
            }

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
                paint.setColor(SkColorSetARGB(highlight, 0xff, 0xff, 0xff));
                canvas.drawPath(
                    SkPath::Circle(center[0], center[1], 32.f), paint);
            }

            float smoothed_state = smooth_between_values(
                ctx,
                data.bits,
                2,
                condition_is_true(selected),
                1.f,
                0.f,
                animated_transition{default_curve, 200});

            rgb8 color = interpolate(
                rgb8(0xa0, 0xa0, 0xa0),
                rgb8(0x90, 0xc0, 0xff),
                smoothed_state);

            auto state = get_widget_state(
                ctx,
                id,
                (is_disabled ? WIDGET_DISABLED : NO_FLAGS)
                    | (is_pressed(data.keyboard_click_state_)
                           ? WIDGET_DEPRESSED
                           : NO_FLAGS));

            render_click_flares(
                ctx, data.bits, data.click_flare_bits, state, center, color);

            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setColor(SkColorSetARGB(0xff, 0xc0, 0xc0, 0xc0));
                paint.setStrokeWidth(3);
                canvas.drawPath(
                    SkPath::Circle(center[0], center[1], 15.f), paint);
            }

            float dot_radius = interpolate(0.f, 10.f, smoothed_state);

            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor(SkColorSetARGB(0xff, 0x90, 0xc0, 0xff));
            paint.setStrokeWidth(3);
            canvas.drawPath(
                SkPath::Circle(center[0], center[1], dot_radius), paint);
        }
    }
    alia_end
}

} // namespace alia
