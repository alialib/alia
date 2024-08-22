#include "alia/ui/context.hpp"
#include "alia/ui/events.hpp"
#include "alia/ui/layout/specification.hpp"
#include "alia/ui/utilities/widgets.hpp"
#include <alia/ui/library/slider.hpp>

#include <algorithm>

#include <alia/ui/color.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/utilities/skia.hpp>

#include <include/core/SkBlurTypes.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>

// This file implements the slider widget, which is declared in ui_library.hpp.

namespace alia {

struct slider_metrics
{
    layout_scalar default_width;
    layout_scalar height, descent;
    layout_box thumb_region;
    box<1, layout_scalar> track_region;
};

constexpr layout_scalar thumb_radius = 16;

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

slider_metrics
get_metrics(ui_context ctx)
{
    slider_metrics metrics;
    metrics.default_width = as_layout_size(resolve_absolute_length(
        get_layout_traversal(ctx), 0, absolute_length(16, EM)));
    layout_scalar x = as_layout_size(resolve_absolute_length(
        get_layout_traversal(ctx), 0, absolute_length(0.5f, EM)));
    metrics.height = x;
    metrics.descent = as_layout_size(x * 0.5);
    metrics.thumb_region = layout_box(
        make_layout_vector(round_to_layout_scalar(x * -0.3), 0),
        make_layout_vector(round_to_layout_scalar(x * 0.6), x));
    box<1, layout_scalar> track_region;
    track_region.corner[0] = as_layout_size(x * 0.5);
    track_region.size[0] = x / 6;
    metrics.track_region = track_region;
    return metrics;
}

void
draw_track(
    dataless_ui_context ctx,
    slider_metrics const& metrics,
    unsigned axis,
    layout_vector const& track_position,
    layout_scalar track_width)
{
    auto& event = cast_event<render_event>(ctx);
    SkCanvas& canvas = *event.canvas;

    layout_vector track_size;
    track_size[axis] = track_width;
    track_size[1 - axis] = metrics.track_region.size[0];

    layout_box track_box(track_position, track_size);

    SkPaint paint;
    paint.setAntiAlias(true);
    rgba8 const color = rgba8(
        0xc0, 0xc0, 0xc0, 0xff); // get_color_property(path, "track-color");
    paint.setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));

    canvas.drawPath(SkPath::Rect(as_skrect(track_box)), paint);
}

void
draw_thumb(
    ui_context ctx,
    slider_metrics const&,
    unsigned, // axis
    layout_vector const& thumb_position,
    widget_state state)
{
    auto& event = cast_event<render_event>(ctx);
    SkCanvas& canvas = *event.canvas;

    {
        SkPaint paint;
        paint.setAntiAlias(true);

        rgba8 const color = rgba8(0x90, 0xc0, 0xff, 0xff);
        // get_color_property(path, "thumb-color");
        paint.setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));

        canvas.drawPath(
            SkPath::Circle(thumb_position[0], thumb_position[1], 16.f), paint);
    }

    uint8_t highlight = 0;
    if ((state.code & WIDGET_PRIMARY_STATE_MASK_CODE) == WIDGET_DEPRESSED_CODE)
    {
        // highlight = 0x40;
        highlight = 0x20;
    }
    else if ((state.code & WIDGET_PRIMARY_STATE_MASK_CODE) == WIDGET_HOT_CODE)
    {
        highlight = 0x20;
    }
    if (highlight != 0)
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(highlight, 0xff, 0xff, 0xff));
        canvas.drawPath(
            SkPath::Circle(thumb_position[0], thumb_position[1], 24.f), paint);
    }
}

struct slider_data
{
    slider_metrics metrics; // TODO: Don't store this?
    layout_leaf layout_node;
    bool dragging = false;
    float dragging_offset;
    double dragging_value;
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
    track_position[1 - axis] = assigned_region.corner[1 - axis]
                               + data.metrics.track_region.corner[0];
    return track_position;
}

static layout_scalar
get_track_width(dataless_ui_context ctx, slider_data& data, unsigned axis)
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
    if (data.dragging
        && (!signal_has_value(value)
            || read_signal(value) == data.dragging_value))
    {
        thumb_position[axis]
            = float(get_mouse_position(ctx)[axis] - data.dragging_offset);
        thumb_position[1 - axis] = get_center(assigned_region)[1 - axis];

        float const maximum_position = get_high_corner(assigned_region)[axis]
                                       - right_side_padding_size(ctx) - 1;
        float const minimum_position
            = assigned_region.corner[axis] + left_side_padding_size(ctx);

        thumb_position[axis] = std::clamp(
            thumb_position[axis], minimum_position, maximum_position);
    }
    else
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
    thumb_region.corner[axis] = thumb_position[axis] - 16;
    thumb_region.corner[1 - axis] = thumb_position[1 - axis] - 16;
    thumb_region.size[axis] = 32;
    thumb_region.size[1 - axis] = 32;
    return thumb_region;
}

void
do_unsafe_slider(
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
            data.metrics = get_metrics(ctx);
            vector<2, float> default_size;
            default_size[axis] = data.metrics.default_width;
            default_size[1 - axis] = data.metrics.height;
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(
                    default_size,
                    default_size[1] - data.metrics.descent,
                    data.metrics.descent),
                LEFT | BASELINE_Y | PADDED);
            add_layout_node(get_layout_traversal(ctx), &data.layout_node);
            break;
        }

        case REGION_CATEGORY: {
            if (!signal_has_value(value))
                break;

            layout_vector track_size;
            track_size[axis] = get_track_width(ctx, data, axis);
            track_size[1 - axis] = data.metrics.track_region.size[0];
            do_box_region(
                ctx,
                track_id,
                add_border(
                    layout_box(
                        get_track_position(ctx, data, axis), track_size),
                    make_layout_vector(2, 2)));

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
                || detect_drag(ctx, track_id, mouse_button::LEFT))
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

            if (detect_drag(ctx, thumb_id, mouse_button::LEFT))
            {
                if (!data.dragging)
                {
                    layout_vector thumb_position = get_thumb_position(
                        ctx, data, axis, minimum, maximum, value);
                    data.dragging_offset = float(
                        get_mouse_position(ctx)[axis] - thumb_position[axis]);
                    data.dragging = true;
                }

                double new_value
                    = (get_integer_mouse_position(ctx)[axis]
                       - data.dragging_offset
                       - data.layout_node.assignment().region.corner[axis]
                       - left_side_padding_size(ctx))
                          * get_values_per_pixel(
                              ctx, data, axis, minimum, maximum)
                      + minimum;

                write_signal(
                    value, round_and_clamp(new_value, minimum, maximum, step));

                data.dragging_value = read_signal(value);
            }

            if (detect_drag_release(ctx, thumb_id, mouse_button::LEFT))
                data.dragging = false;

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
            draw_track(
                ctx,
                data.metrics,
                axis,
                get_track_position(ctx, data, axis),
                get_track_width(ctx, data, axis));
            widget_state thumb_state = get_widget_state(ctx, thumb_id);
            if (signal_has_value(value))
            {
                draw_thumb(
                    ctx,
                    data.metrics,
                    axis,
                    get_thumb_position(
                        ctx, data, axis, minimum, maximum, value),
                    thumb_state);
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
}

} // namespace alia