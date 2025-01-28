#include <alia/ui/library/node_expander.hpp>

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

struct node_expander_bit_layout
{
    click_flare_bit_layout click_flare;
    smoothing_bit_field state_smoothing;
};

struct node_expander_data
{
    bitpack<node_expander_bit_layout> bits;
    keyboard_click_state keyboard_click_state_;
    layout_leaf layout_node;
};

struct node_expander_style_info
{
    rgb8 normal_color;
    rgb8 disabled_color;
    rgb8 highlight_color;
};

node_expander_style_info
extract_node_expander_style_info(dataless_ui_context ctx)
{
    auto const& theme = get_system(ctx).theme;
    return node_expander_style_info{
        .normal_color = theme.foreground[4],
        .disabled_color = lerp(theme.background[4], theme.foreground[4], 0.4f),
        .highlight_color = theme.primary[4]};
}

void
render_node_expander(
    dataless_ui_context ctx,
    node_expander_data& data,
    bool expanded,
    interaction_status interaction_status,
    node_expander_style_info const& style)
{
    auto& event = cast_event<render_event>(ctx);

    SkCanvas& canvas = *event.canvas;

    auto const& region = data.layout_node.assignment().region;

    SkRect rect = as_skrect(region);

    if (event.canvas->quickReject(rect))
        return;

    auto center = get_center(region);

    auto position = region.corner;

    float smoothed_state = smooth_between_values(
        ctx,
        ALIA_BITREF(data.bits, state_smoothing),
        condition_is_true(expanded),
        1.f,
        0.f,
        animated_transition{default_curve, 200});

    canvas.save();

    canvas.translate(
        position[0] + region.size[0] / SkIntToScalar(2),
        position[1] + region.size[1] / SkIntToScalar(2));
    canvas.rotate(lerp(0.f, 90.f, smoothed_state));

    {
        SkPaint paint;
        paint.setAntiAlias(true);
        if (is_disabled(interaction_status))
        {
            paint.setColor(as_skcolor(style.disabled_color));
        }
        else
        {
            paint.setColor(as_skcolor(style.normal_color));
        }
        paint.setStyle(SkPaint::kFill_Style);
        SkScalar a = region.size[0] / SkDoubleToScalar(2);
        SkPath path;
        path.incReserve(4);
        SkPoint p0;
        p0.fX = a * SkDoubleToScalar(-0.34);
        p0.fY = a * SkDoubleToScalar(-0.5);
        path.moveTo(p0);
        SkPoint p1;
        p1.fX = p0.fX;
        p1.fY = a * SkDoubleToScalar(0.5);
        path.lineTo(p1);
        SkPoint p2;
        p2.fX = p0.fX + a * SkDoubleToScalar(0.866);
        p2.fY = 0;
        path.lineTo(p2);
        path.lineTo(p0);
        canvas.drawPath(path, paint);
    }

    canvas.restore();

    if (is_disabled(interaction_status))
        return;

    if (is_active(interaction_status) || is_hovered(interaction_status))
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(
            0x20,
            style.highlight_color.r,
            style.highlight_color.g,
            style.highlight_color.b));
        canvas.drawPath(SkPath::Circle(center[0], center[1], 28.f), paint);
    }

    render_click_flares(
        ctx,
        ALIA_NESTED_BITPACK(data.bits, click_flare),
        interaction_status,
        center,
        style.highlight_color,
        28);
}

void
do_node_expander(
    ui_context ctx, duplex<bool> expanded, layout const& layout_spec)
{
    node_expander_data* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    auto const id = data_ptr;

    bool const is_disabled = !signal_ready_to_write(expanded);

    alia_untracked_switch(get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(make_layout_vector(48, 48), 30, 18),
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
                if (signal_has_value(expanded)
                    && signal_ready_to_write(expanded))
                {
                    write_signal(expanded, !read_signal(expanded));
                }
                abort_traversal(ctx);
            }

            break;

        case RENDER_CATEGORY: {
            auto const style = extract_node_expander_style_info(ctx);
            auto interaction_status = get_interaction_status(
                ctx,
                id,
                (is_disabled ? WIDGET_DISABLED : NO_FLAGS)
                    | (is_pressed(data.keyboard_click_state_) ? WIDGET_ACTIVE
                                                              : NO_FLAGS));
            render_node_expander(
                ctx,
                data,
                condition_is_true(expanded),
                interaction_status,
                style);
        }
    }
    alia_end
}

} // namespace alia
