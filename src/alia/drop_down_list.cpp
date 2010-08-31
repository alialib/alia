#include <alia/drop_down_list.hpp>
#include <cctype>

namespace alia {

struct drop_down_button_data
{
    drop_down_button_data() : key_state(0) {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
    int key_state;
};

bool do_drop_down_button(context& ctx, layout const& layout_spec,
    flag_set flags, region_id id)
{
    if (!id) id = get_region_id(ctx);
    drop_down_button_data& data = *get_data<drop_down_button_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i size = ctx.artist->get_minimum_drop_down_button_size();
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, FILL, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_drop_down_button(data.artist_data,
            data.layout_data.assigned_region,
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
        break;
      }
    }
    return false;
}

}
