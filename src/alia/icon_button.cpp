#include <alia/icon_button.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/widget_state.hpp>

namespace alia {

struct icon_button_data
{
    icon_button_data() : key_state(0) {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
    int key_state;
};

icon_button_result
do_icon_button(
    context& ctx,
    standard_icon icon,
    layout const& layout_spec,
    region_id id)
{
    if (!id) id = get_region_id(ctx);
    icon_button_data& data = *get_data<icon_button_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i size = ctx.artist->get_icon_button_size(data.artist_data,
            icon);
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, LEFT | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_icon_button(data.artist_data, icon,
            data.layout_data.assigned_region.corner,
            get_widget_state(ctx, id, true, data.key_state == 1));
        break;
      }
     case REGION_CATEGORY:
        do_region(ctx, id, data.layout_data.assigned_region);
        break;
     case INPUT_CATEGORY:
      {
        add_to_focus_order(ctx, id);
        return detect_click(ctx, id, LEFT_BUTTON) ||
            detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
      }
    }
    return false;
}

}
