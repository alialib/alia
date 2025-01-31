#include <alia/ui/library/radio_button.hpp>

#include <alia/core/bit_packing.hpp>
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

struct radio_button_bit_layout
{
    click_flare_bit_layout click_flare;
    smoothing_bit_field state_smoothing;
};

struct radio_button_data
{
    bitpack<radio_button_bit_layout> bits;
    keyboard_click_state keyboard_click_state_;
    layout_leaf layout_node;
};

struct radio_button_style_info
{
    rgb8 disabled_color;
    rgb8 highlight_color;
    rgb8 outline_color;
    rgb8 dot_color;
};

radio_button_style_info
extract_radio_button_style_info(dataless_ui_context ctx)
{
    auto const& theme = get_system(ctx).theme;
    return {
        .disabled_color
        = lerp(theme.background.base.main, theme.structural.base.main, 0.4f),
        .highlight_color = theme.primary.stronger[1].main,
        .outline_color = theme.structural.base.main,
        .dot_color = theme.primary.stronger[1].main};
}

void
render_radio_button(
    dataless_ui_context ctx,
    radio_button_data& data,
    bool selected,
    interaction_status state,
    radio_button_style_info const& style)
{
    auto& event = cast_event<render_event>(ctx);
    SkCanvas& canvas = *event.canvas;

    auto const& region = data.layout_node.assignment().region;

    SkRect rect;
    rect.fLeft = SkScalar(region.corner[0]);
    rect.fTop = SkScalar(region.corner[1]);
    rect.fRight = SkScalar(region.corner[0] + region.size[0]);
    rect.fBottom = SkScalar(region.corner[1] + region.size[1]);

    if (event.canvas->quickReject(rect))
        return;

    auto center = get_center(region);

    if (is_disabled(state))
    {
        {
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setColor(as_skcolor(style.disabled_color));
            paint.setStrokeWidth(3);
            canvas.drawPath(SkPath::Circle(center[0], center[1], 15.f), paint);
        }

        if (selected)
        {
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor(as_skcolor(style.disabled_color));
            paint.setStrokeWidth(3);
            canvas.drawPath(SkPath::Circle(center[0], center[1], 10), paint);
        }

        return;
    }

    if (is_active(state) || is_hovered(state))
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(as_skcolor(rgba8(style.highlight_color, 0x20)));
        canvas.drawPath(SkPath::Circle(center[0], center[1], 32.f), paint);
    }

    float smoothed_state = smooth_between_values(
        ctx,
        ALIA_BITREF(data.bits, state_smoothing),
        selected,
        1.f,
        0.f,
        animated_transition{default_curve, 200});

    render_click_flares(
        ctx,
        ALIA_NESTED_BITPACK(data.bits, click_flare),
        state,
        center,
        style.dot_color);

    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setColor(as_skcolor(style.outline_color));
        paint.setStrokeWidth(3);
        canvas.drawPath(SkPath::Circle(center[0], center[1], 15.f), paint);
    }

    float dot_radius = lerp(0.f, 10.f, smoothed_state);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(as_skcolor(style.dot_color));
    paint.setStrokeWidth(3);
    canvas.drawPath(SkPath::Circle(center[0], center[1], dot_radius), paint);
}

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
                return;

            add_to_focus_order(ctx, id);

            if (detect_click(ctx, id, mouse_button::LEFT)
                || detect_keyboard_click(ctx, data.keyboard_click_state_, id))
            {
                // TODO: It's not necessarily the left mouse button.
                fire_click_flare(
                    ctx, ALIA_NESTED_BITPACK(data.bits, click_flare));
                if (signal_ready_to_write(selected))
                    write_signal(selected, true);
                abort_traversal(ctx);
            }

            break;

        case RENDER_CATEGORY: {
            auto state = get_interaction_status(
                ctx,
                id,
                (is_disabled ? WIDGET_DISABLED : NO_FLAGS)
                    | (is_pressed(data.keyboard_click_state_) ? WIDGET_ACTIVE
                                                              : NO_FLAGS));
            auto style = extract_radio_button_style_info(ctx);
            render_radio_button(
                ctx, data, condition_is_true(selected), state, style);
            break;
        }
    }
    alia_end
}

} // namespace alia
