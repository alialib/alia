#include <alia/ui/library/slider.hpp>

#include <algorithm>

#include <alia/ui/color.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/utilities/skia.hpp>

#include <include/core/SkBlurTypes.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>

// This file implements the slider widget, which is declared in ui_library.hpp.

namespace alia {

constexpr layout_scalar thumb_radius = 16;
constexpr layout_scalar track_width = 6;

inline layout_scalar
left_side_padding_size(dataless_ui_context)
{
    return thumb_radius;
}

inline layout_scalar
right_side_padding_size(dataless_ui_context)
{
    return thumb_radius;
}

struct slider_style_info
{
    rgb8 highlight_color;
    rgb8 track_color;
    rgb8 thumb_color;
};

void
draw_track(
    dataless_ui_context ctx,
    unsigned axis,
    layout_vector const& track_position,
    layout_scalar track_length,
    slider_style_info const& style)
{
    auto& event = cast_event<render_event>(ctx);
    SkCanvas& canvas = *event.canvas;

    layout_vector track_size;
    track_size[axis] = track_length;
    track_size[1 - axis] = track_width;

    layout_box track_box(track_position, track_size);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(as_skcolor(style.track_color));

    canvas.drawPath(SkPath::Rect(as_skrect(track_box)), paint);
}

void
draw_thumb(
    dataless_ui_context ctx,
    unsigned, // axis
    layout_vector const& thumb_position,
    interaction_status state,
    slider_style_info const& style)
{
    auto& event = cast_event<render_event>(ctx);
    SkCanvas& canvas = *event.canvas;

    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(as_skcolor(style.thumb_color));
        canvas.drawPath(
            SkPath::Circle(thumb_position[0], thumb_position[1], 16.f), paint);
    }

    if (is_active(state) || is_hovered(state))
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(
            as_skcolor(rgba8(get_system(ctx).theme.foreground[4], 0x20)));
        canvas.drawPath(
            SkPath::Circle(thumb_position[0], thumb_position[1], 24.f), paint);
    }
}

struct slider_data
{
    layout_leaf layout_node;
};

static double
round_and_clamp(double x, double min, double max, double step)
{
    assert(min <= max);
    double clamped = std::clamp(x, min, max);
    if (step != 0)
        return std::floor((clamped - min) / step + 0.5) * step + min;
    else
        return clamped;
}

static double
get_values_per_pixel(
    dataless_ui_context ctx,
    slider_data& data,
    unsigned axis,
    double minimum,
    double maximum)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    return double(maximum - minimum)
         / (assigned_region.size[axis] - left_side_padding_size(ctx)
            - right_side_padding_size(ctx) - 1);
}

static layout_vector
get_track_position(dataless_ui_context ctx, slider_data& data, unsigned axis)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector track_position;
    track_position[axis]
        = assigned_region.corner[axis] + left_side_padding_size(ctx);
    track_position[1 - axis]
        = assigned_region.corner[1 - axis]
        + (assigned_region.size[1 - axis] - track_width) / 2;
    return track_position;
}

static layout_scalar
get_track_length(dataless_ui_context ctx, slider_data& data, unsigned axis)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    return assigned_region.size[axis] - left_side_padding_size(ctx)
         - right_side_padding_size(ctx);
}

static layout_vector
get_thumb_position(
    dataless_ui_context ctx,
    slider_data& data,
    unsigned axis,
    double minimum,
    double maximum,
    readable<double> value)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector thumb_position;
    // if (data.dragging
    //     && (!signal_has_value(value)
    //         || read_signal(value) == data.dragging_value))
    // {
    //     thumb_position[axis] = get_mouse_position(ctx)[axis];
    //     thumb_position[1 - axis] = get_center(assigned_region)[1 - axis];

    //     float const maximum_position =
    //     get_high_corner(assigned_region)[axis]
    //                                    - right_side_padding_size(ctx) - 1;
    //     float const minimum_position
    //         = assigned_region.corner[axis] + left_side_padding_size(ctx);

    //     thumb_position[axis] = std::clamp(
    //         thumb_position[axis], minimum_position, maximum_position);
    // }
    // else
    {
        thumb_position[1 - axis] = get_center(assigned_region)[1 - axis];
        thumb_position[axis]
            = assigned_region.corner[axis]
            + round_to_layout_scalar(
                  (read_signal(value) - minimum)
                  / get_values_per_pixel(ctx, data, axis, minimum, maximum))
            + left_side_padding_size(ctx);
    }
    return thumb_position;
}

static layout_box
get_thumb_region(
    dataless_ui_context ctx,
    slider_data& data,
    unsigned axis,
    double minimum,
    double maximum,
    readable<double> value)
{
    layout_vector thumb_position
        = get_thumb_position(ctx, data, axis, minimum, maximum, value);
    layout_box thumb_region;
    thumb_region.corner[axis] = thumb_position[axis] - thumb_radius * 1.25f;
    thumb_region.corner[1 - axis]
        = thumb_position[1 - axis] - thumb_radius * 1.25f;
    thumb_region.size[axis] = thumb_radius * 2.5f;
    thumb_region.size[1 - axis] = thumb_radius * 2.5f;
    return thumb_region;
}

slider_style_info
extract_slider_style_info(dataless_ui_context ctx)
{
    auto const& theme = get_system(ctx).theme;
    return {
        .highlight_color = theme.primary[4],
        .track_color = theme.foreground[4],
        .thumb_color = theme.primary[7],
    };
}

void
render_slider(
    dataless_ui_context ctx,
    unsigned axis,
    slider_data& data,
    duplex<double> value,
    double minimum,
    double maximum,
    interaction_status thumb_status,
    slider_style_info const& style)
{
    draw_track(
        ctx,
        axis,
        get_track_position(ctx, data, axis),
        get_track_length(ctx, data, axis),
        style);
    if (signal_has_value(value))
    {
        draw_thumb(
            ctx,
            axis,
            get_thumb_position(ctx, data, axis, minimum, maximum, value),
            thumb_status,
            style);
    }
    // TODO: Draw focus
    // if (thumb_state & WIDGET_FOCUSED)
    // {
    //     draw_focus_rect(
    //         ctx,
    //         data.focus_rendering,
    //         data.layout_node.assignment().region);
    // }
}

void
do_slider(
    ui_context ctx,
    duplex<double> value,
    double minimum,
    double maximum,
    double step,
    layout const& layout_spec,
    slider_flag_set flags)
{
    unsigned axis = (flags & SLIDER_VERTICAL) ? 1 : 0;

    auto& data = get_cached_data<slider_data>(ctx);

    auto track_id = offset_id(&data, 0);
    auto thumb_id = offset_id(&data, 1);

    switch (get_event_category(ctx))
    {
        case REFRESH_CATEGORY: {
            // TODO: Figure out refresh/caching/styling logic.
            vector<2, float> default_size;
            auto const default_length = as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(16, EM)));
            auto default_height = as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(0.5f, EM)));
            default_size[axis] = default_length;
            default_size[1 - axis] = default_height;
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(default_size, 0, 0),
                LEFT | BASELINE_Y | PADDED);
            add_layout_node(get_layout_traversal(ctx), &data.layout_node);
            break;
        }

        case REGION_CATEGORY: {
            if (!signal_has_value(value))
                break;

            do_box_region(ctx, track_id, data.layout_node.assignment().region);

            do_box_region(
                ctx,
                thumb_id,
                get_thumb_region(ctx, data, axis, minimum, maximum, value));

            break;
        }

        case INPUT_CATEGORY: {
            if (!signal_has_value(value))
                break;

            if (detect_mouse_press(ctx, track_id, mouse_button::LEFT)
                || detect_drag(ctx, track_id, mouse_button::LEFT)
                || detect_mouse_press(ctx, thumb_id, mouse_button::LEFT)
                || detect_drag(ctx, thumb_id, mouse_button::LEFT))
            {
                double new_value
                    = (get_mouse_position(ctx)[axis]
                       - data.layout_node.assignment().region.corner[axis]
                       - left_side_padding_size(ctx))
                        * get_values_per_pixel(
                            ctx, data, axis, minimum, maximum)
                    + minimum;

                write_signal(
                    value, round_and_clamp(new_value, minimum, maximum, step));

                // TODO: Set focus!!
                // set_focus(ctx, thumb_id);
            }

            add_to_focus_order(ctx, thumb_id);

            // TODO: Handle keyboard input!
            /*
            key_event_info info;
            if (detect_key_press(ctx, &info, thumb_id) && info.mods == 0)
            {
                double increment = (maximum - minimum) / 10;
                switch (info.code)
                {
                    case KEY_LEFT:
                        if (axis == 0)
                        {
                            write_signal(
                                value,
                                round_and_clamp(
                                    get(value) - increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_DOWN:
                        if (axis == 1)
                        {
                            set_new_value(
                                value,
                                result,
                                round_and_clamp(
                                    get(value) - increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_RIGHT:
                        if (axis == 0)
                        {
                            set_new_value(
                                value,
                                result,
                                round_and_clamp(
                                    get(value) + increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_UP:
                        if (axis == 1)
                        {
                            set_new_value(
                                value,
                                result,
                                round_and_clamp(
                                    get(value) + increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_HOME:
                        set_new_value(value, result, minimum);
                        acknowledge_input_event(ctx);
                        break;
                    case KEY_END:
                        set_new_value(value, result, maximum);
                        acknowledge_input_event(ctx);
                        break;
                }
            }*/

            break;
        }
        case RENDER_CATEGORY:
            auto const style = extract_slider_style_info(ctx);
            interaction_status thumb_status
                = get_interaction_status(ctx, thumb_id);
            render_slider(
                ctx, axis, data, value, minimum, maximum, thumb_status, style);
            break;
    }
}

} // namespace alia
