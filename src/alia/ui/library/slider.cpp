#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

// This file implements the slider widget, which is declared in ui_library.hpp.

namespace alia {

struct slider_renderer : dispatch_interface
{
    virtual layout_scalar default_width(ui_context& ctx) const = 0;
    virtual layout_scalar left_border(ui_context& ctx) const = 0;
    virtual layout_scalar right_border(ui_context& ctx) const = 0;
    virtual layout_scalar height(ui_context& ctx) const = 0;
    virtual layout_box thumb_region(ui_context& ctx) const = 0;
    virtual box<1,layout_scalar> track_region(ui_context& ctx) const = 0;

    virtual void draw_track(
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& track_position, layout_scalar track_width) const
        = 0;

    virtual void draw_thumb(
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& thumb_position, widget_state state) const = 0;
};

struct default_slider_renderer : slider_renderer
{
    struct data_type
    {
        caching_renderer_data track, thumb;
    };

    layout_scalar default_width(ui_context& ctx) const
    {
        return as_layout_size(resolve_absolute_length(
            get_layout_traversal(ctx), 0, absolute_length(20, EM)));
    }
    layout_scalar left_border(ui_context& ctx) const
    {
        return as_layout_size(resolve_absolute_length(
            get_layout_traversal(ctx), 0, absolute_length(0.5f, EM)));
    }
    layout_scalar right_border(ui_context& ctx) const
    {
        return as_layout_size(resolve_absolute_length(
            get_layout_traversal(ctx), 0, absolute_length(0.5f, EM)));
    }
    layout_scalar height(ui_context& ctx) const
    {
        return as_layout_size(resolve_absolute_length(
            get_layout_traversal(ctx), 0, absolute_length(1.3f, EM)));
    }
    layout_box thumb_region(ui_context& ctx) const
    {
        layout_scalar x =
            as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(1.3f, EM)));
        return layout_box(make_layout_vector(
            round_to_layout_scalar(x * -0.3), 0),
            make_layout_vector(round_to_layout_scalar(x * 0.6), x));
    }
    box<1,layout_scalar> track_region(ui_context& ctx) const
    {
        layout_scalar x =
            as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 0, absolute_length(1.3f, EM)));
        box<1,layout_scalar> b;
        b.corner[0] = as_layout_size(x * 0.5);
        b.size[0] = x / 6;
        return b;
    }

    void draw_track(
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& track_position, layout_scalar track_width) const
    {
        data_type* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector track_size;
        track_size[axis] = track_width;
        track_size[1 - axis] = track_region(ctx).size[0];

        layout_box track_box(track_position, track_size);

        caching_renderer cache(ctx, data->track, ref(*ctx.style.id),
            track_box);
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
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& thumb_position, widget_state state) const
    {
        data_type* data;
        cast_data_ptr(&data, data_ptr);

        layout_box thumb_region = this->thumb_region(ctx);
        thumb_region.corner += thumb_position;

        caching_renderer cache(ctx, data->thumb,
            combine_ids(ref(*ctx.style.id), make_id(state)),
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
    themed_rendering_data<slider_renderer> rendering;
    layout_leaf layout_node;
    widget_identity track_id, thumb_id;
    bool dragging;
    int dragging_offset;
    double dragging_value;
    focus_rect_data focus_rendering;
};

static double clamp(double x, double min, double max, double step)
{
    assert(min <= max);
    double clamped = clamp(x, min, max);
    if (step != 0)
        return std::floor((clamped - min) / step + 0.5) * step + min;
    else
        return clamped;
}

static double get_values_per_pixel(ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    slider_renderer const* renderer = data.rendering.renderer;
    return double(maximum - minimum) /
        (assigned_region.size[axis] - renderer->left_border(ctx) -
        renderer->right_border(ctx) - 1);
}

static layout_vector get_track_position(ui_context& ctx, slider_data& data,
    unsigned axis)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector track_position;
    track_position[axis] = assigned_region.corner[axis] +
        renderer->left_border(ctx);
    track_position[1 - axis] = assigned_region.corner[1 - axis] +
        renderer->track_region(ctx).corner[0];
    return track_position;
}

static layout_scalar get_track_width(ui_context& ctx, slider_data& data,
    unsigned axis)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_box const& assigned_region = data.layout_node.assignment().region;
    return assigned_region.size[axis] -
        renderer->left_border(ctx) - renderer->right_border(ctx);
}

static layout_vector get_thumb_position(ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum, getter<double> const& value)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector thumb_position;
    if (data.dragging &&
        (!value.is_gettable() || value.get() == data.dragging_value))
    {
        thumb_position[axis] = get_integer_mouse_position(ctx)[axis] -
            data.dragging_offset;
        thumb_position[1 - axis] = assigned_region.corner[1 - axis];

        int const maximum_position = get_high_corner(assigned_region)[axis] -
            renderer->right_border(ctx) - 1;
        int const minimum_position = assigned_region.corner[axis] +
            renderer->left_border(ctx);

        thumb_position[axis] = clamp(thumb_position[axis],
            minimum_position, maximum_position);
    }
    else
    {
        thumb_position = assigned_region.corner;
        thumb_position[axis] +=
            round_to_layout_scalar((get(value) - minimum) /
                get_values_per_pixel(ctx, data, axis, minimum, maximum)) +
            renderer->left_border(ctx);
    }
    return thumb_position;
}

static layout_box get_thumb_region(ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum, getter<double> const& value)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_vector thumb_position =
        get_thumb_position(ctx, data, axis, minimum, maximum, value);
    layout_box thumb_region;
    thumb_region.corner[axis] =
        renderer->thumb_region(ctx).corner[0] + thumb_position[axis];
    thumb_region.corner[1 - axis] =
        renderer->thumb_region(ctx).corner[1] + thumb_position[1 - axis];
    thumb_region.size[axis] = renderer->thumb_region(ctx).size[0];
    thumb_region.size[1 - axis] = renderer->thumb_region(ctx).size[1];
    return thumb_region;
}

slider_result
do_slider(ui_context& ctx, accessor<double> const& value,
    double minimum, double maximum, double step,
    layout const& layout_spec, slider_flag_set flags)
{
    slider_result result;
    result.changed = false;

    slider_data* data;
    get_cached_data(ctx, &data);

    unsigned axis = (flags & SLIDER_VERTICAL) ? 1 : 0;

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_slider_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data->rendering, &default_renderer);
        slider_renderer const* renderer = data->rendering.renderer;
        vector<2,int> default_size;
        default_size[axis] = renderer->default_width(ctx);
        default_size[1 - axis] = renderer->height(ctx);
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(default_size, 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        slider_renderer const* renderer = data->rendering.renderer;
        renderer->draw_track(ctx, data->rendering.data, axis,
            get_track_position(ctx, *data, axis),
            get_track_width(ctx, *data, axis));
        widget_state thumb_state = get_widget_state(ctx, &data->thumb_id);
        if (value.is_gettable())
        {
            renderer->draw_thumb(ctx, data->rendering.data, axis,
                get_thumb_position(ctx, *data, axis, minimum, maximum, value),
                thumb_state);
        }
        if (thumb_state & WIDGET_FOCUSED)
        {
            draw_focus_rect(ctx, data->focus_rendering,
                data->layout_node.assignment().region);
        }
        break;
      }

     case REGION_CATEGORY:
      {
        if (!value.is_gettable())
            break;

        slider_renderer const* renderer = data->rendering.renderer;

        layout_vector track_size;
        track_size[axis] = get_track_width(ctx, *data, axis);
        track_size[1 - axis] = renderer->track_region(ctx).size[0];
        do_box_region(ctx, &data->track_id,
            add_border(
                layout_box(get_track_position(ctx, *data, axis), track_size),
                make_layout_vector(2, 2)));

        do_box_region(ctx, &data->thumb_id,
            get_thumb_region(ctx, *data, axis, minimum, maximum, value));

        break;
      }

     case INPUT_CATEGORY:
      {
        if (!value.is_gettable())
            break;

        if (detect_mouse_press(ctx, &data->track_id, LEFT_BUTTON) ||
            detect_drag(ctx, &data->track_id, LEFT_BUTTON))
        {
            slider_renderer const* renderer = data->rendering.renderer;

            double new_value =
                (get_integer_mouse_position(ctx)[axis] -
                    data->layout_node.assignment().region.corner[axis] -
                    renderer->left_border(ctx)) *
                get_values_per_pixel(ctx, *data, axis, minimum, maximum) +
                minimum;

            set_new_value(value, result,
                clamp(new_value, minimum, maximum, step));

            set_focus(ctx, &data->thumb_id);
        }

        if (detect_drag(ctx, &data->thumb_id, LEFT_BUTTON))
        {
            if (!data->dragging)
            {
                layout_vector thumb_position =
                    get_thumb_position(ctx, *data, axis, minimum, maximum,
                        value);
                data->dragging_offset =
                    get_integer_mouse_position(ctx)[axis] -
                    thumb_position[axis];
                data->dragging = true;
            }

            slider_renderer const* renderer = data->rendering.renderer;

            double new_value =
                (get_integer_mouse_position(ctx)[axis] -
                    data->dragging_offset -
                    data->layout_node.assignment().region.corner[axis] -
                    renderer->left_border(ctx)) *
                get_values_per_pixel(ctx, *data, axis, minimum, maximum) +
                minimum;

            set_new_value(value, result,
                clamp(new_value, minimum, maximum, step));

            data->dragging_value = get(value);
        }

        if (detect_drag_release(ctx, &data->thumb_id, LEFT_BUTTON))
            data->dragging = false;

        add_to_focus_order(ctx, &data->thumb_id);

        key_event_info info;
        if (detect_key_press(ctx, &info, &data->thumb_id) && info.mods == 0)
        {
            double increment = (maximum - minimum) / 10;
            switch (info.code)
            {
             case KEY_LEFT:
                if (axis == 0)
                {
                    set_new_value(value, result,
                        clamp(get(value) - increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_DOWN:
                if (axis == 1)
                {
                    set_new_value(value, result,
                        clamp(get(value) - increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_RIGHT:
                if (axis == 0)
                {
                    set_new_value(value, result,
                        clamp(get(value) + increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_UP:
                if (axis == 1)
                {
                    set_new_value(value, result,
                        clamp(get(value) + increment, minimum, maximum, step));
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

    return result;
}

}
