#include <alia/slider.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/artist.hpp>
#include <alia/surface.hpp>
#include <alia/widget_state.hpp>
#include <alia/timer.hpp>
#include <alia/region.hpp>

namespace alia {

namespace impl {

struct slider_data
{
    slider_data() : dragging(false) {}
    alia::layout_data layout_data;
    region_data track_id0, track_id1, thumb_id;
    bool dragging;
    int dragging_offset;
    double dragging_value;
    artist_data_ptr track_data, thumb_data;
};

bool do_slider_track_input(context& ctx, region_id id)
{
    static int const delay_after_first_increment = 400;
    static int const delay_after_other_increment = 40;
    if (detect_mouse_down(ctx, id, LEFT_BUTTON))
    {
        start_timer(ctx, id, delay_after_first_increment);
        return true;
    }
    else if (detect_click_in_progress(ctx, id, LEFT_BUTTON)
        && is_timer_done(ctx, id))
    {
        restart_timer(ctx, id, delay_after_other_increment);
        return true;
    }
    return false;
}

template<typename T>
static T clamp(T x, T min, T max)
{
    assert(min <= max);
    return (std::min)((std::max)(x, min), max);
}

bool do_slider(context& ctx, double* value, double minimum, double maximum,
    double step, bool integer, flag_set flags, layout const& layout_spec)
{
    slider_data& data = *get_data<slider_data>(ctx);

    artist& artist = *ctx.artist;

    if (step == 0)
        step = (std::max)(integer ? 1. : 0., (maximum - minimum) / 10);

    box2i const& assigned_region = data.layout_data.assigned_region;

    // TODO
    unsigned axis = flags.code & 1;

    double const scale = double(maximum - minimum) /
        (assigned_region.size[axis] - artist.get_slider_left_border() -
        artist.get_slider_right_border() - 1);

    point2i thumb_position;
    if (data.dragging && *value == data.dragging_value)
    {
        thumb_position[axis] = ctx.pass_state.integer_mouse_position[axis] -
            data.dragging_offset;
        thumb_position[1 - axis] = assigned_region.corner[1 - axis];

        int const maximum_position = get_high_corner(assigned_region)[axis] -
            artist.get_slider_right_border() - 1;
        int const minimum_position = assigned_region.corner[axis] +
            artist.get_slider_left_border();

        thumb_position[axis] = clamp(thumb_position[axis],
            minimum_position, maximum_position);
    }
    else
    {
        thumb_position = assigned_region.corner;
        thumb_position[axis] += int((*value - minimum) / scale + 0.5) +
            artist.get_slider_left_border();
    }

    box2i thumb_region;
    thumb_region.corner[axis] =
        artist.get_slider_thumb_region().corner[0] + thumb_position[axis];
    thumb_region.corner[1 - axis] =
        artist.get_slider_thumb_region().corner[1] + thumb_position[1 - axis];
    thumb_region.size[axis] = artist.get_slider_thumb_region().size[0];
    thumb_region.size[1 - axis] = artist.get_slider_thumb_region().size[1];

    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        if (ctx.event->type == REFRESH_EVENT)
        {
            refresh_region_id(ctx, &data.track_id0);
            refresh_region_id(ctx, &data.track_id1);
            refresh_region_id(ctx, &data.thumb_id);
        }
        vector2i size;
        size[axis] = artist.get_default_slider_width();
        size[1 - axis] = artist.get_slider_height();
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, LEFT | CENTER_Y, true));
        break;
      }

     case RENDER_CATEGORY:
      {
        point2i track_position;
        track_position[axis] = assigned_region.corner[axis] +
            artist.get_slider_left_border();
        track_position[1 - axis] = assigned_region.corner[1 - axis] +
            artist.get_slider_track_region().corner[0];
        int track_width = assigned_region.size[axis] -
            artist.get_slider_left_border() - artist.get_slider_right_border();
        artist.draw_slider_track(data.track_data, axis, track_width,
            track_position);
        artist.draw_slider_thumb(data.thumb_data, axis, thumb_position,
            get_widget_state(ctx, &data.thumb_id));

        break;
      }

     case REGION_CATEGORY:
      {
        box2i track_region0;
        track_region0.corner = assigned_region.corner;
        track_region0.corner[1 - axis] +=
            artist.get_slider_track_hot_region().corner[0];
        track_region0.size[axis] =
            thumb_region.corner[axis] - assigned_region.corner[axis];
        track_region0.size[1 - axis] =
            artist.get_slider_track_hot_region().size[0];
        do_region(ctx, &data.track_id0, track_region0);

        box2i track_region1;
        track_region1.corner[axis] = get_high_corner(thumb_region)[axis];
        track_region1.corner[1 - axis] =
            assigned_region.corner[1 - axis] +
            artist.get_slider_track_hot_region().corner[0];
        track_region1.size[axis] =
            get_high_corner(assigned_region)[axis] -
            get_high_corner(thumb_region)[axis];
        track_region1.size[1 - axis] =
            artist.get_slider_track_hot_region().size[0];
        do_region(ctx, &data.track_id1, track_region1);

        do_region(ctx, &data.thumb_id, thumb_region);

        break;
      }

     case INPUT_CATEGORY:
      {
        static int const delay_after_first_increment = 400;
        static int const delay_after_other_increment = 40;

        if (do_slider_track_input(ctx, &data.track_id0))
        {
            *value = clamp(*value - step, minimum, maximum);
            return true;
        }

        if (do_slider_track_input(ctx, &data.track_id1))
        {
            *value = clamp(*value + step, minimum, maximum);
            return true;
        }

        if (detect_drag(ctx, &data.thumb_id, LEFT_BUTTON))
        {
            if (!data.dragging)
            {
                data.dragging_offset =
                    ctx.pass_state.integer_mouse_position[axis] -
                    thumb_position[axis];
                data.dragging = true;
            }

            *value = (ctx.pass_state.integer_mouse_position[axis] -
                data.dragging_offset - assigned_region.corner[axis] -
                artist.get_slider_left_border()) * scale + minimum;

            *value = clamp(*value, minimum, maximum);

            data.dragging_value = *value;

            return true;
        }

        if (detect_drag_release(ctx, &data.thumb_id, LEFT_BUTTON))
            data.dragging = false;

        add_to_focus_order(ctx, &data.thumb_id);

        key_event_info info;
        if (detect_key_press(ctx, &info, &data.thumb_id) && info.mods == 0)
        {
            switch (info.code)
            {
             case KEY_LEFT:
                if (axis == 0)
                {
                    *value = clamp(*value - step, minimum, maximum);
                    acknowledge_input_event(ctx);
                }
                return true;
             case KEY_DOWN:
                if (axis == 1)
                {
                    *value = clamp(*value - step, minimum, maximum);
                    acknowledge_input_event(ctx);
                }
                return true;
             case KEY_RIGHT:
                if (axis == 0)
                {
                    *value = clamp(*value + step, minimum, maximum);
                    acknowledge_input_event(ctx);
                }
                return true;
             case KEY_UP:
                if (axis == 1)
                {
                    *value = clamp(*value + step, minimum, maximum);
                    acknowledge_input_event(ctx);
                }
                return true;
             case KEY_HOME:
                *value = minimum;
                acknowledge_input_event(ctx);
                return true;
             case KEY_END:
                *value = maximum;
                acknowledge_input_event(ctx);
                return true;
             default:
                ;
            }
        }

        break;
      }
    }

    return false;
}

}

}
