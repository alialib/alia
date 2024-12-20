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

struct checkbox_style_info
{
    rgb8 highlight_color;
    rgb8 disabled_fill_color;
    rgb8 disabled_check_color;
    rgb8 disabled_outline_color;
    rgb8 outline_color;
    rgb8 checked_fill_color;
    rgb8 check_color;
};

checkbox_style_info
extract_checkbox_style_info(dataless_ui_context ctx)
{
    auto const& theme = get_system(ctx).theme;
    return {
        .highlight_color = theme.primary,
        .disabled_fill_color
        = interpolate(theme.surface, theme.on_surface, 0.4f),
        .disabled_check_color = theme.surface,
        .disabled_outline_color
        = interpolate(theme.surface, theme.on_surface, 0.4f),
        .outline_color = theme.on_surface,
        .checked_fill_color = theme.primary,
        .check_color = theme.on_primary,
    };
}

void
render_checkbox(
    dataless_ui_context ctx,
    checkbox_data& data,
    bool checked,
    widget_state state,
    checkbox_style_info const& style)
{
    auto& event = cast_event<render_event>(ctx);

    SkCanvas& canvas = *event.canvas;

    auto const& region = data.layout_node.assignment().region;

    SkRect rect = as_skrect(region);

    if (event.canvas->quickReject(rect))
        return;

    if (is_disabled(state))
    {
        float const padding = 11.f;
        auto checkbox_rect = remove_border(
            region,
            box_border_width<float>{padding, padding, padding, padding});

        if (condition_is_true(checked))
        {
            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setStyle(SkPaint::kStrokeAndFill_Style);
                paint.setColor(as_skcolor(style.disabled_fill_color));
                paint.setStrokeWidth(4);
                canvas.drawPath(
                    SkPath::RRect(as_skrect(checkbox_rect), 2, 2), paint);
            }
            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setColor(as_skcolor(style.disabled_check_color));
                paint.setStrokeWidth(4);
                paint.setStrokeCap(SkPaint::kSquare_Cap);
                SkPath path;
                path.incReserve(3);
                SkPoint p0;
                p0.fX = checkbox_rect.corner[0] + checkbox_rect.size[0] * 0.2f;
                p0.fY = checkbox_rect.corner[1] + checkbox_rect.size[1] * 0.5f;
                path.moveTo(p0);
                SkPoint p1;
                p1.fX = checkbox_rect.corner[0] + checkbox_rect.size[0] * 0.4f;
                p1.fY = checkbox_rect.corner[1] + checkbox_rect.size[1] * 0.7f;
                path.lineTo(p1);
                SkPoint p2;
                p2.fX = checkbox_rect.corner[0] + checkbox_rect.size[0] * 0.8f;
                p2.fY = checkbox_rect.corner[1] + checkbox_rect.size[1] * 0.3f;
                path.lineTo(p2);
                canvas.drawPath(path, paint);
            }
        }
        else
        {
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setColor(as_skcolor(style.disabled_outline_color));
            paint.setStrokeWidth(4);
            canvas.drawPath(
                SkPath::RRect(as_skrect(checkbox_rect), 2, 2), paint);
        }

        return;
    }

    auto center = get_center(region);

    float smoothed_state = smooth_between_values(
        ctx,
        ALIA_BITREF(data.bits, state_smoothing),
        condition_is_true(checked),
        1.f,
        0.f,
        animated_transition{default_curve, 200});

    if (is_depressed(state) || is_hot(state))
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(as_skcolor(rgba8(style.highlight_color, 0x20)));
        canvas.drawPath(SkPath::Circle(center[0], center[1], 32.f), paint);
    }

    float const padding = 11.f;
    auto checkbox_rect = remove_border(
        region, box_border_width<float>{padding, padding, padding, padding});

    if (condition_is_true(checked))
    {
        {
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setStyle(SkPaint::kStrokeAndFill_Style);
            paint.setColor(as_skcolor(style.checked_fill_color));
            paint.setStrokeWidth(4);
            canvas.drawPath(
                SkPath::RRect(as_skrect(checkbox_rect), 2, 2), paint);
        }
        {
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setColor(as_skcolor(style.check_color));
            paint.setStrokeWidth(4);
            paint.setStrokeCap(SkPaint::kSquare_Cap);
            SkPath path;
            path.incReserve(3);
            SkPoint p0;
            p0.fX = checkbox_rect.corner[0] + checkbox_rect.size[0] * 0.2f;
            p0.fY = checkbox_rect.corner[1] + checkbox_rect.size[1] * 0.5f;
            path.moveTo(p0);
            SkPoint p1;
            p1.fX = checkbox_rect.corner[0] + checkbox_rect.size[0] * 0.4f;
            p1.fY = checkbox_rect.corner[1] + checkbox_rect.size[1] * 0.7f;
            path.lineTo(p1);
            SkPoint p2;
            p2.fX = checkbox_rect.corner[0] + checkbox_rect.size[0] * 0.8f;
            p2.fY = checkbox_rect.corner[1] + checkbox_rect.size[1] * 0.3f;
            path.lineTo(p2);
            canvas.drawPath(path, paint);
        }
    }
    else
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setColor(as_skcolor(style.outline_color));
        paint.setStrokeWidth(4);
        canvas.drawPath(SkPath::RRect(as_skrect(checkbox_rect), 2, 2), paint);
    }

    rgb8 color = interpolate(
        style.outline_color, style.checked_fill_color, smoothed_state);

    render_click_flares(
        ctx,
        ALIA_NESTED_BITPACK(data.bits, click_flare),
        state,
        center,
        color);
}

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
                leaf_layout_requirements(make_layout_vector(48, 48), 32, 16),
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
            auto interaction_status = get_widget_state(
                ctx,
                id,
                (is_disabled ? WIDGET_DISABLED : NO_FLAGS)
                    | (is_pressed(data.keyboard_click_state_)
                           ? WIDGET_DEPRESSED
                           : NO_FLAGS));
            auto style = extract_checkbox_style_info(ctx);
            render_checkbox(
                ctx,
                data,
                condition_is_true(checked),
                interaction_status,
                style);
        }
    }
    alia_end
}

} // namespace alia
