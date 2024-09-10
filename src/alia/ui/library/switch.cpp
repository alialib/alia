#include <alia/ui/library/switch.hpp>

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
#include <include/core/SkMaskFilter.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>

namespace alia {

struct switch_data
{
    unsigned bits;
    keyboard_click_state keyboard_click_state_;
    layout_leaf layout_node;
};

void
do_switch(ui_context ctx, duplex<bool> state, layout const& layout_spec)
{
    switch_data* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    auto const id = data_ptr;

    bool const is_disabled = !signal_ready_to_write(state);

    alia_untracked_switch(get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(make_layout_vector(60, 35), 0, 0),
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
                // TODO: Fix mouse_button::LEFT
                fire_click_flare(ctx, mouse_button::LEFT, data.bits, 4);
                if (signal_has_value(state) && signal_ready_to_write(state))
                {
                    write_signal(state, !read_signal(state));
                }
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

            if (is_disabled)
            {
                {
                    auto const track_color = rgb8(0x48, 0x48, 0x4b);
                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setColor(SkColorSetARGB(
                        0xff, track_color.r, track_color.g, track_color.b));
                    paint.setStyle(SkPaint::kStroke_Style);
                    paint.setStrokeCap(SkPaint::kRound_Cap);
                    paint.setStrokeWidth(16);
                    canvas.drawPath(
                        SkPath::Line(
                            SkPoint::Make(
                                region.corner[0] + region.size[0] * 0.25f,
                                get_center(region)[1]),
                            SkPoint::Make(
                                region.corner[0] + region.size[0] * 0.75f,
                                get_center(region)[1])),
                        paint);
                }

                {
                    float const dot_x_offset
                        = condition_is_true(state) ? 0.75f : 0.25f;

                    float dot_radius = condition_is_true(state) ? 16.f : 14.f;

                    auto const color = rgb8(0x60, 0x60, 0x66);
                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setColor(
                        SkColorSetARGB(0xff, color.r, color.g, color.b));
                    canvas.drawPath(
                        SkPath::Circle(
                            region.corner[0] + region.size[0] * dot_x_offset,
                            get_center(region)[1],
                            dot_radius),
                        paint);
                }

                break;
            }

            float switch_position = smooth_between_values(
                ctx,
                data.bits,
                0,
                condition_is_true(state),
                1.f,
                0.f,
                animated_transition{default_curve, 200});

            float dot_radius = interpolate(14.f, 16.f, switch_position);

            float const dot_x_offset
                = interpolate(0.25f, 0.75f, switch_position);

            rgb8 color = interpolate(
                rgb8(0xd6, 0xd6, 0xd6),
                rgb8(0x90, 0xc0, 0xff),
                switch_position);

            rgb8 track_color = interpolate(
                rgb8(0x64, 0x64, 0x64),
                rgb8(0x64, 0x64, 0x64),
                switch_position);

            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setColor(SkColorSetARGB(
                    0xff, track_color.r, track_color.g, track_color.b));
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                paint.setStrokeWidth(16);
                canvas.drawPath(
                    SkPath::Line(
                        SkPoint::Make(
                            region.corner[0] + region.size[0] * 0.25f,
                            get_center(region)[1]),
                        SkPoint::Make(
                            region.corner[0] + region.size[0] * 0.75f,
                            get_center(region)[1])),
                    paint);
            }

            {
                const SkScalar blurSigma = 3.0f;
                const SkScalar xDrop = 2.0f;
                const SkScalar yDrop = 2.0f;

                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setColor(
                    SkColorSetARGB(0x40, color.r, color.g, color.b));
                paint.setMaskFilter(SkMaskFilter::MakeBlur(
                    kNormal_SkBlurStyle, blurSigma, false));

                canvas.drawPath(
                    SkPath::Circle(
                        region.corner[0] + region.size[0] * dot_x_offset
                            + xDrop,
                        get_center(region)[1] + yDrop,
                        dot_radius),
                    paint);
            }

            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setColor(
                    SkColorSetARGB(0xff, color.r, color.g, color.b));
                canvas.drawPath(
                    SkPath::Circle(
                        region.corner[0] + region.size[0] * dot_x_offset,
                        get_center(region)[1],
                        dot_radius),
                    paint);
            }

            auto interaction_status = get_widget_state(
                ctx,
                id,
                (is_disabled ? WIDGET_DISABLED : NO_FLAGS)
                    | (is_pressed(data.keyboard_click_state_)
                           ? WIDGET_DEPRESSED
                           : NO_FLAGS));

            uint8_t highlight = 0;
            if (is_click_in_progress(ctx, id, mouse_button::LEFT)
                || is_pressed(data.keyboard_click_state_))
            {
                highlight = 0x40;
            }
            else if (is_click_possible(ctx, id))
            {
                highlight = 0x40;
            }
            if (highlight != 0)
            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setColor(
                    SkColorSetARGB(highlight, color.r, color.g, color.b));
                canvas.drawPath(
                    SkPath::Circle(
                        region.corner[0] + region.size[0] * dot_x_offset,
                        get_center(region)[1],
                        24),
                    paint);
            }

            render_click_flares(
                ctx,
                data.bits,
                4,
                interaction_status,
                make_vector(
                    region.corner[0] + region.size[0] * dot_x_offset,
                    get_center(region)[1]),
                color,
                24);
        }
    }
    alia_end
}

} // namespace alia
