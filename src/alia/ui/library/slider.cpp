#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

// This file implements the slider widget, which is declared in ui_library.hpp.

namespace alia {

struct slider_metrics
{
    layout_scalar default_width;
    layout_scalar height, descent;
    layout_box thumb_region;
    box<1,layout_scalar> track_region;
    // border on either side of the track
    layout_scalar left_border, right_border;
};

struct slider_renderer : dispatch_interface
{
    virtual slider_metrics get_metrics(ui_context& ctx) const = 0;

    virtual void draw_track(
        ui_context& ctx, slider_metrics const& metrics, unsigned axis,
        layout_vector const& track_position, layout_scalar track_width) const
        = 0;

    virtual void draw_thumb(
        ui_context& ctx, slider_metrics const& metrics, unsigned axis,
        layout_vector const& thumb_position, widget_state state) const = 0;
};

struct default_slider_renderer : slider_renderer
{
    slider_metrics get_metrics(ui_context& ctx) const
    {
        slider_metrics metrics;
        metrics.default_width =
            as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(20, EM)));
        metrics.left_border =
            as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(0.5f, EM)));
        metrics.right_border =
            as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(0.5f, EM)));
        layout_scalar x =
            as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(1.3f, EM)));
        metrics.height = x;
        metrics.descent = as_layout_size(x * 0.2);
        metrics.thumb_region =
            layout_box(make_layout_vector(
                round_to_layout_scalar(x * -0.3), 0),
                make_layout_vector(round_to_layout_scalar(x * 0.6), x));
        box<1,layout_scalar> track_region;
        track_region.corner[0] = as_layout_size(x * 0.5);
        track_region.size[0] = x / 6;
        metrics.track_region = track_region;
        return metrics;
    }

    void draw_track(
        ui_context& ctx, slider_metrics const& metrics, unsigned axis,
        layout_vector const& track_position, layout_scalar track_width) const
    {
        ALIA_GET_CACHED_DATA(caching_renderer_data);

        if (!is_render_pass(ctx))
            return;

        layout_vector track_size;
        track_size[axis] = track_width;
        track_size[1 - axis] = metrics.track_region.size[0];

        layout_box track_box(track_position, track_size);

        caching_renderer cache(ctx, data, ref(ctx.style.id), track_box);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), track_box.size);

            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0, "slider");

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            rgba8 color = get_color_property(path, "track-color");

            renderer.canvas().drawColor(
                SkColorSetARGB(color.a, color.r, color.g, color.b));

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }

    void draw_thumb(
        ui_context& ctx, slider_metrics const& metrics, unsigned axis,
        layout_vector const& thumb_position, widget_state state) const
    {
        ALIA_GET_CACHED_DATA(caching_renderer_data);

        if (!is_render_pass(ctx))
            return;

        layout_box thumb_region = metrics.thumb_region;
        thumb_region.corner += thumb_position;

        caching_renderer cache(ctx, data,
            combine_ids(ref(ctx.style.id), make_id(state)),
            thumb_region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), thumb_region.size);

            stateful_style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0,
                    "slider", state);

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            rgba8 color = get_color_property(path, "thumb-color");

            renderer.canvas().drawColor(
                SkColorSetARGB(color.a, color.r, color.g, color.b));

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

struct slider_data
{
    slider_data() : dragging(false) {}
    themed_rendering_data rendering;
    keyed_data<slider_metrics> metrics;
    layout_leaf layout_node;
    widget_identity track_id, thumb_id;
    bool dragging;
    int dragging_offset;
    double dragging_value;
    focus_rect_data focus_rendering;
};

static double
round_and_clamp(double x, double min, double max, double step)
{
    assert(min <= max);
    double clamped = clamp(x, min, max);
    if (step != 0)
        return std::floor((clamped - min) / step + 0.5) * step + min;
    else
        return clamped;
}

static double
get_values_per_pixel(dataless_ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    return double(maximum - minimum) /
        (assigned_region.size[axis] - get(data.metrics).left_border -
        get(data.metrics).right_border - 1);
}

static layout_vector
get_track_position(dataless_ui_context& ctx, slider_data& data, unsigned axis)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector track_position;
    track_position[axis] =
        assigned_region.corner[axis] + get(data.metrics).left_border;
    track_position[1 - axis] = assigned_region.corner[1 - axis] +
        get(data.metrics).track_region.corner[0];
    return track_position;
}

static layout_scalar
get_track_width(dataless_ui_context& ctx, slider_data& data, unsigned axis)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    return assigned_region.size[axis] -
        get(data.metrics).left_border - get(data.metrics).right_border;
}

static layout_vector
get_thumb_position(dataless_ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum, accessor<double> const& value)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector thumb_position;
    if (data.dragging &&
        (!value.is_gettable() || value.get() == data.dragging_value))
    {
        thumb_position[axis] = get_integer_mouse_position(ctx)[axis] -
            data.dragging_offset;
        thumb_position[1 - axis] = assigned_region.corner[1 - axis];

        int const maximum_position = get_high_corner(assigned_region)[axis] -
            get(data.metrics).right_border - 1;
        int const minimum_position = assigned_region.corner[axis] +
            get(data.metrics).left_border;

        thumb_position[axis] = clamp(thumb_position[axis],
            minimum_position, maximum_position);
    }
    else
    {
        thumb_position = assigned_region.corner;
        thumb_position[axis] +=
            round_to_layout_scalar((get(value) - minimum) /
                get_values_per_pixel(ctx, data, axis, minimum, maximum)) +
            get(data.metrics).left_border;
    }
    return thumb_position;
}

static layout_box
get_thumb_region(dataless_ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum,
    accessor<double> const& value)
{
    layout_vector thumb_position =
        get_thumb_position(ctx, data, axis, minimum, maximum, value);
    layout_box thumb_region;
    thumb_region.corner[axis] =
        get(data.metrics).thumb_region.corner[0] + thumb_position[axis];
    thumb_region.corner[1 - axis] =
        get(data.metrics).thumb_region.corner[1] + thumb_position[1 - axis];
    thumb_region.size[axis] = get(data.metrics).thumb_region.size[0];
    thumb_region.size[1 - axis] = get(data.metrics).thumb_region.size[1];
    return thumb_region;
}

slider_result
do_unsafe_slider(ui_context& ctx, accessor<double> const& value,
    double minimum, double maximum, double step,
    layout const& layout_spec, slider_flag_set flags)
{
    slider_result result;
    result.changed = false;

    unsigned axis = (flags & SLIDER_VERTICAL) ? 1 : 0;

    ALIA_GET_CACHED_DATA(slider_data)

    slider_renderer const* renderer;
    static default_slider_renderer default_renderer;
    get_themed_renderer(ctx, data.rendering, &renderer, &default_renderer);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        refresh_keyed_data(data.metrics, *ctx.style.id);
        if (!is_valid(data.metrics))
        {
            scoped_data_block block(ctx, data.rendering.refresh_block);
            set(data.metrics, renderer->get_metrics(ctx));
        }
        vector<2,int> default_size;
        default_size[axis] = get(data.metrics).default_width;
        default_size[1 - axis] = get(data.metrics).height;
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(default_size,
                default_size[1] - get(data.metrics).descent,
                get(data.metrics).descent),
            LEFT | BASELINE_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case REGION_CATEGORY:
      {
        if (!value.is_gettable())
            break;

        layout_vector track_size;
        track_size[axis] = get_track_width(ctx, data, axis);
        track_size[1 - axis] = get(data.metrics).track_region.size[0];
        do_box_region(ctx, &data.track_id,
            add_border(
                layout_box(get_track_position(ctx, data, axis), track_size),
                make_layout_vector(2, 2)));

        do_box_region(ctx, &data.thumb_id,
            get_thumb_region(ctx, data, axis, minimum, maximum, value));

        break;
      }

     case INPUT_CATEGORY:
      {
        if (!value.is_gettable())
            break;

        if (detect_mouse_press(ctx, &data.track_id, LEFT_BUTTON) ||
            detect_drag(ctx, &data.track_id, LEFT_BUTTON))
        {
            double new_value =
                (get_integer_mouse_position(ctx)[axis] -
                    data.layout_node.assignment().region.corner[axis] -
                    get(data.metrics).left_border) *
                get_values_per_pixel(ctx, data, axis, minimum, maximum) +
                minimum;

            set_new_value(value, result,
                round_and_clamp(new_value, minimum, maximum, step));

            set_focus(ctx, &data.thumb_id);
        }

        if (detect_drag(ctx, &data.thumb_id, LEFT_BUTTON))
        {
            if (!data.dragging)
            {
                layout_vector thumb_position =
                    get_thumb_position(ctx, data, axis, minimum, maximum,
                        value);
                data.dragging_offset =
                    get_integer_mouse_position(ctx)[axis] -
                    thumb_position[axis];
                data.dragging = true;
            }

            double new_value =
                (get_integer_mouse_position(ctx)[axis] -
                    data.dragging_offset -
                    data.layout_node.assignment().region.corner[axis] -
                    get(data.metrics).left_border) *
                get_values_per_pixel(ctx, data, axis, minimum, maximum) +
                minimum;

            set_new_value(value, result,
                round_and_clamp(new_value, minimum, maximum, step));

            data.dragging_value = get(value);
        }

        if (detect_drag_release(ctx, &data.thumb_id, LEFT_BUTTON))
            data.dragging = false;

        add_to_focus_order(ctx, &data.thumb_id);

        key_event_info info;
        if (detect_key_press(ctx, &info, &data.thumb_id) && info.mods == 0)
        {
            double increment = (maximum - minimum) / 10;
            switch (info.code)
            {
             case KEY_LEFT:
                if (axis == 0)
                {
                    set_new_value(value, result, round_and_clamp(
                        get(value) - increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_DOWN:
                if (axis == 1)
                {
                    set_new_value(value, result, round_and_clamp(
                        get(value) - increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_RIGHT:
                if (axis == 0)
                {
                    set_new_value(value, result, round_and_clamp(
                        get(value) + increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_UP:
                if (axis == 1)
                {
                    set_new_value(value, result, round_and_clamp(
                        get(value) + increment, minimum, maximum, step));
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
        }

        break;
      }
    }

    alia_tracked_block(data.rendering.drawing_block)
    {
        renderer->draw_track(ctx, get(data.metrics), axis,
            get_track_position(ctx, data, axis),
            get_track_width(ctx, data, axis));
        widget_state thumb_state = get_widget_state(ctx, &data.thumb_id);
        if (value.is_gettable())
        {
            renderer->draw_thumb(ctx, get(data.metrics), axis,
                get_thumb_position(ctx, data, axis, minimum, maximum, value),
                thumb_state);
        }
        if (is_render_pass(ctx) && thumb_state & WIDGET_FOCUSED)
        {
            draw_focus_rect(ctx, data.focus_rendering,
                data.layout_node.assignment().region);
        }
    }
    alia_end

    return result;
}

}
