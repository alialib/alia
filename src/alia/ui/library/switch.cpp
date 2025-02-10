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

struct switch_bit_layout
{
    click_flare_bit_layout click_flare;
    smoothing_bit_field state_smoothing;
};

struct switch_data
{
    bitpack<switch_bit_layout> bits;
    keyboard_click_state keyboard_click_state_;
    layout_leaf layout_node;
};

struct switch_style_info
{
    // track color when switch is disabled
    rgb8 disabled_track_color;
    // dot color when switch is disabled
    rgb8 disabled_dot_color;
    // track color when switch is off
    rgb8 off_track_color;
    // track color when switch is on
    rgb8 on_track_color;
    // dot color when switch is off
    rgb8 off_dot_color;
    // dot color when switch is on
    rgb8 on_dot_color;
    // color of hover/press highlight
    rgb8 highlight_color;
};

switch_style_info
extract_switch_style_info(dataless_ui_context ctx)
{
    auto const& theme = get_system(ctx).theme;
    return {
        .disabled_track_color
        = lerp(theme.background.base.main, theme.structural.base.main, 0.5f),
        .disabled_dot_color
        = lerp(theme.background.base.main, theme.structural.base.main, 0.6f),
        .off_track_color = theme.structural.weaker[0].main,
        .on_track_color = theme.structural.base.main,
        .off_dot_color = theme.structural.stronger[0].main,
        .on_dot_color = theme.accent.base.main,
        .highlight_color = theme.accent.base.main,
    };
}

void
render_switch(
    dataless_ui_context ctx,
    switch_data& data,
    bool state,
    interaction_status interaction_status,
    switch_style_info const& style)
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

    if (is_disabled(interaction_status))
    {
        {
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor(as_skcolor(style.disabled_track_color));
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

            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor(as_skcolor(style.disabled_dot_color));
            canvas.drawPath(
                SkPath::Circle(
                    region.corner[0] + region.size[0] * dot_x_offset,
                    get_center(region)[1],
                    dot_radius),
                paint);
        }

        return;
    }

    float switch_position = smooth_between_values(
        ctx,
        ALIA_BITREF(data.bits, state_smoothing),
        condition_is_true(state),
        1.f,
        0.f,
        animated_transition{default_curve, 200});

    float dot_radius = lerp(14.f, 16.f, switch_position);

    float const dot_x_offset = lerp(0.25f, 0.75f, switch_position);

    rgb8 const dot_color
        = lerp(style.off_dot_color, style.on_dot_color, switch_position);

    rgb8 const track_color
        = lerp(style.off_track_color, style.on_track_color, switch_position);

    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(
            SkColorSetARGB(0xff, track_color.r, track_color.g, track_color.b));
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
        const SkScalar blur_sigma = 4.0f;
        const SkScalar x_drop = 2.0f;
        const SkScalar y_drop = 2.0f;

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(
            SkColorSetARGB(0x80, dot_color.r, dot_color.g, dot_color.b));
        paint.setMaskFilter(
            SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur_sigma, false));

        canvas.drawPath(
            SkPath::Circle(
                region.corner[0] + region.size[0] * dot_x_offset + x_drop,
                get_center(region)[1] + y_drop,
                dot_radius),
            paint);
    }

    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(
            SkColorSetARGB(0xff, dot_color.r, dot_color.g, dot_color.b));
        canvas.drawPath(
            SkPath::Circle(
                region.corner[0] + region.size[0] * dot_x_offset,
                get_center(region)[1],
                dot_radius),
            paint);
    }

    if (is_active(interaction_status) || is_hovered(interaction_status))
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(
            0x40,
            style.highlight_color.r,
            style.highlight_color.g,
            style.highlight_color.b));
        canvas.drawPath(
            SkPath::Circle(
                region.corner[0] + region.size[0] * dot_x_offset,
                get_center(region)[1],
                24),
            paint);
    }

    render_click_flares(
        ctx,
        ALIA_NESTED_BITPACK(data.bits, click_flare),
        interaction_status,
        make_vector(
            region.corner[0] + region.size[0] * dot_x_offset,
            get_center(region)[1]),
        dot_color,
        24);
}

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
                leaf_layout_requirements(make_layout_vector(60, 35), 25, 10),
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
                fire_click_flare(
                    ctx, ALIA_NESTED_BITPACK(data.bits, click_flare));
                if (signal_has_value(state) && signal_ready_to_write(state))
                {
                    write_signal(state, !read_signal(state));
                }
                abort_traversal(ctx);
            }

            break;

        case RENDER_CATEGORY: {
            auto interaction_status = get_interaction_status(
                ctx,
                id,
                (is_disabled ? WIDGET_DISABLED : NO_FLAGS)
                    | (is_pressed(data.keyboard_click_state_) ? WIDGET_ACTIVE
                                                              : NO_FLAGS));
            auto style = extract_switch_style_info(ctx);
            render_switch(
                ctx,
                data,
                condition_is_true(state),
                interaction_status,
                style);
        }
    }
    alia_end
}

} // namespace alia
