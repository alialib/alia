#include <alia/draggable_separator.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/artist.hpp>
#include <alia/input_utils.hpp>

namespace alia {

struct draggable_separator_data
{
    artist_data_ptr artist_data;
    alia::layout_data layout_data;
    int drag_start_delta;
};

bool do_draggable_separator(context& ctx, int* value, flag_set flags,
    layout const& layout_spec, region_id id)
{
    int axis = (flags & Y_AXIS) ? 1 : 0;
    int const drag_axis = 1 - axis;
    if (!id) id = get_region_id(ctx);
    draggable_separator_data& data = *get_data<draggable_separator_data>(ctx);
    artist& artist = *ctx.artist;
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i minimum_size(artist.get_separator_width(),
            artist.get_separator_width());
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(minimum_size, 0, 0, minimum_size,
            axis ? CENTER_X | FILL_Y : FILL_X | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        box2i const& assigned_region = data.layout_data.assigned_region;
        point2i const& corner = assigned_region.corner + (axis ?
            vector2i((assigned_region.size[0] -
                artist.get_separator_width()) / 2, 0) :
            vector2i(0, (assigned_region.size[1] -
                artist.get_separator_width()) / 2));
        int length = axis ? assigned_region.size[1] :
            assigned_region.size[0];
        artist.draw_separator(data.artist_data, corner, axis, length);
        if (detect_potential_click(ctx, id) ||
            detect_click_in_progress(ctx, id, LEFT_BUTTON))
        {
            ctx.surface->set_mouse_cursor(drag_axis ? UP_DOWN_ARROW_CURSOR :
                LEFT_RIGHT_ARROW_CURSOR);
        }
        break;
      }
     case REGION_CATEGORY:
      {
        box2i region = data.layout_data.assigned_region;
        region.corner[0] -= 1;
        region.size[0] += 2;
        do_region(ctx, id, region);
        break;
      }
     case INPUT_CATEGORY:
        if (detect_mouse_down(ctx, id, LEFT_BUTTON))
        {
            data.drag_start_delta =
                get_integer_mouse_position(ctx)[drag_axis] - *value;
        }
        if (detect_drag(ctx, id, LEFT_BUTTON))
        {
            *value = get_integer_mouse_position(ctx)[drag_axis] -
                data.drag_start_delta;
            return true;
        }
        break;
    }
    return false;
}

}
