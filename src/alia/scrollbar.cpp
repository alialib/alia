#include <alia/scrollbar.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/input_utils.hpp>
#include <alia/box.hpp>
#include <alia/timer.hpp>

namespace alia {

void refresh_scrollbar_data(context& ctx, scrollbar_data& data)
{
    refresh_region_id(ctx, &data.background_id_data[0]);
    refresh_region_id(ctx, &data.background_id_data[1]);
    refresh_region_id(ctx, &data.thumb_id_data);
    refresh_region_id(ctx, &data.button_id_data[0]);
    refresh_region_id(ctx, &data.button_id_data[1]);
}

class scrollbar
{
 public:
    scrollbar(context& ctx, scrollbar_data& data,
        box2i const& area, int axis, int content_size, int window_size,
        int position, int line_increment, int page_increment);

    void do_pass();

    int get_position() const { return logical_position; }

 private:
    void set_logical_position(int position);

    int get_max_logical_position() const;

    void set_physical_position(int position);

    widget_state get_button_state(region_id id, box2i const& area);
    widget_state get_thumb_state(region_id id, box2i const& area);

    void process_button_input(region_id id,
        box2i const& area, int increment);

    context& ctx;
    scrollbar_data& data;
    bool enabled;
    box2i area;
    int axis, content_size, window_size, logical_position, line_increment,
        page_increment, max_physical_position;
};

scrollbar::scrollbar(context& ctx, scrollbar_data& data,
    box2i const& area, int axis, int content_size, int window_size,
    int position, int line_increment, int page_increment)
  : ctx(ctx)
  , data(data)
  , area(area)
  , axis(axis)
  , content_size(content_size)
  , window_size(window_size)
  , logical_position(position)
  , line_increment(line_increment)
  , page_increment(page_increment)
{
}

template<typename T>
static T clamp(T x, T min, T max)
{
    assert(min <= max);
    return (std::min)((std::max)(x, min), max);
}

void scrollbar::set_logical_position(int position)
{
    logical_position = clamp(position, 0, get_max_logical_position());

    data.physical_position = clamp(logical_position *
        max_physical_position / get_max_logical_position(), 0,
        max_physical_position);
}

int scrollbar::get_max_logical_position() const
{
    return content_size - window_size;
}

void scrollbar::set_physical_position(int position)
{
    data.physical_position = clamp(position, 0, max_physical_position);

    logical_position = max_physical_position <= 0 ? 0 :
        clamp(data.physical_position * get_max_logical_position() /
        max_physical_position, 0, get_max_logical_position());
}

widget_state scrollbar::get_button_state(region_id id,
    box2i const& area)
{
    if (detect_click_in_progress(ctx, id, LEFT_BUTTON))
    {
        return widget_states::DEPRESSED;
    }
    else if (detect_potential_click(ctx, id))
    {
        return widget_states::HOT;
    }
    else
        return widget_states::NORMAL;
}

widget_state scrollbar::get_thumb_state(region_id id,
    box2i const& area)
{
    if (detect_drag_in_progress(ctx, id, LEFT_BUTTON))
    {
        return widget_states::DEPRESSED;
    }
    else if (detect_potential_click(ctx, id))
    {
        return widget_states::HOT;
    }
    else
        return widget_states::NORMAL;
}

void scrollbar::process_button_input(region_id id,
    box2i const& area, int increment)
{
    static int const delay_after_first_increment = 400;
    static int const delay_after_other_increment = 40;

    if (detect_mouse_down(ctx, id, LEFT_BUTTON))
    {
        set_logical_position(logical_position + increment);
        start_timer(ctx, id, delay_after_first_increment);
    }
    else if (detect_click_in_progress(ctx, id, LEFT_BUTTON) &&
        is_timer_done(ctx, id))
    {
        set_logical_position(logical_position + increment);
        restart_timer(ctx, id, delay_after_other_increment);
    }
}

void scrollbar::do_pass()
{
    switch (ctx.event->category)
    {
     case RENDER_CATEGORY:
     case REGION_CATEGORY:
     case INPUT_CATEGORY:
        break;
     case LAYOUT_CATEGORY:
        if (ctx.event->type != REFRESH_EVENT)
            break;
     default:
        return;
    }

    // TODO: This should never actually happen, but it does.
    if (content_size <= 0 || window_size <= 0)
        return;

    if (window_size >= content_size)
    {
        enabled = false;
        return;
    }
    else
        enabled = true;

    assert(axis == 0 || axis == 1);
    int major_axis = axis;
    int minor_axis = 1 - axis;

    // If this data was previously used for a scrollbar on a different axis,
    // we need to clear out the cached artist data.
    if (data.axis != major_axis)
    {
        data.background_data[0].reset();
        data.background_data[0].reset();
        data.thumb_data.reset();
        data.button_data[0].reset();
        data.button_data[1].reset();
        data.axis = major_axis;
    }

    artist& artist = *ctx.artist;

    vector2i button_size(artist.get_scrollbar_width(),
        artist.get_scrollbar_button_length());

    box2i button0_area = area;
    button0_area.size[major_axis] = button_size[1];
    region_id button0_id = &data.button_id_data[0];

    box2i button1_area = area;
    button1_area.corner[major_axis] = get_high_corner(area)[major_axis] -
        button_size[1];
    button1_area.size[major_axis] = button_size[1];
    region_id button1_id = &data.button_id_data[1];

    box2i full_bg_area = area;
    full_bg_area.corner[major_axis] += button_size[1];
    full_bg_area.size[major_axis] -= button_size[1] * 2;

    box2i thumb_area = full_bg_area;
    thumb_area.size[major_axis] = std::max(
        artist.get_minimum_scrollbar_thumb_length(),
        window_size * full_bg_area.size[major_axis] / content_size);

    max_physical_position = full_bg_area.size[major_axis] -
        thumb_area.size[major_axis];

    if (max_physical_position < 0)
    {
        if (ctx.event->type == RENDER_EVENT && area.size[0] > 0 &&
            area.size[1] > 0)
        {
            artist.draw_scrollbar_background(data.background_data[0],
                area, axis, 0, widget_states::NORMAL);
        }
        return;
    }

    // Check that the physical position is consistent with the logical
    // position.  If not, it's set accordingly.  The physical position is
    // only there to provide extra precision.
    int position = logical_position;
    set_physical_position(data.physical_position);
    if (position != logical_position)
        set_logical_position(position);

    thumb_area.corner[major_axis] = full_bg_area.corner[major_axis] +
        data.physical_position;
    region_id thumb_id = &data.thumb_id_data;

    int thumb_center = data.physical_position +
        thumb_area.size[major_axis] / 2;

    box2i bg0_area = full_bg_area;
    bg0_area.size[major_axis] = thumb_area.corner[major_axis] -
        full_bg_area.corner[major_axis];
    region_id bg0_id = &data.background_id_data[0];

    box2i bg1_area = full_bg_area;
    bg1_area.corner[major_axis] = get_high_corner(thumb_area)[major_axis];
    bg1_area.size[major_axis] = get_high_corner(full_bg_area)[major_axis] -
        bg1_area.corner[major_axis];
    region_id bg1_id = &data.background_id_data[1];

    switch (ctx.event->category)
    {
     case RENDER_CATEGORY:
      {
        if (bg0_area.size[major_axis] != 0)
        {
            artist.draw_scrollbar_background(data.background_data[0],
                bg0_area, axis, 0, get_button_state(bg0_id, bg0_area));
        }
        if (bg1_area.size[major_axis] != 0)
        {
            artist.draw_scrollbar_background(data.background_data[1],
                bg1_area, axis, 1, get_button_state(bg1_id, bg1_area));
        }
        artist.draw_scrollbar_thumb(data.thumb_data,
            thumb_area, axis, get_thumb_state(thumb_id, thumb_area));
        artist.draw_scrollbar_button(data.button_data[0],
            button0_area.corner, axis, 0,
            get_button_state(button0_id, button0_area));
        artist.draw_scrollbar_button(data.button_data[1],
            button1_area.corner, axis, 1,
            get_button_state(button1_id, button1_area));
        break;
      }

     case REGION_CATEGORY:
      {
        do_region(ctx, bg0_id, bg0_area);
        do_region(ctx, bg1_id, bg1_area);
        do_region(ctx, thumb_id, thumb_area);
        do_region(ctx, button0_id, button0_area);
        do_region(ctx, button1_id, button1_area);
        break;
      }

     case INPUT_CATEGORY:
      {
        process_button_input(bg0_id, bg0_area, -page_increment);
        process_button_input(bg1_id, bg1_area, page_increment);

        if (detect_mouse_down(ctx, thumb_id, LEFT_BUTTON))
        {
            data.drag_start_delta = data.physical_position -
                get_integer_mouse_position(ctx)[major_axis];
        }
        if (detect_drag(ctx, thumb_id, LEFT_BUTTON))
        {
            set_physical_position(get_integer_mouse_position(ctx)[major_axis] +
                data.drag_start_delta);
        }

        process_button_input(button0_id, button0_area, -line_increment);
        process_button_input(button1_id, button1_area, line_increment);

        if (mouse_is_inside_region(ctx, area))
        {
            int wheel_movement = detect_wheel_movement(ctx);
            if (wheel_movement != 0)
            {
                set_logical_position(logical_position - wheel_movement *
                    line_increment);
                acknowledge_input_event(ctx);
            }
        }
      }
    }
}

int do_scrollbar(context& ctx, scrollbar_data& data,
    box2i const& area, int axis, int content_size, int window_size,
    int position, int line_increment, int page_increment)
{
    if (page_increment < 0)
    {
        page_increment = (std::max)(line_increment,
            window_size - line_increment);
    }
    scrollbar sb(ctx, data, area, axis, content_size,
        window_size, position, line_increment, page_increment);
    sb.do_pass();
    return sb.get_position();
}

}
