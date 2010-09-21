#include <alia/clickable_panel.hpp>
#include <alia/context.hpp>
#include <alia/input_utils.hpp>
#include <alia/surface.hpp>

namespace alia {

struct clickable_panel_data
{
    clickable_panel_data() : key_state(0) {}
    int key_state;
    artist_data_ptr focus_rect_data;
};

void clickable_panel::begin(
    context& ctx, layout const& layout_spec, flag_set flags, region_id id)
{
    if (!id) id = get_region_id(ctx);
    clickable_panel_data& data = *get_data<clickable_panel_data>(ctx);
    panel_.begin(ctx, /*ctx.artist->get_code_for_style(ITEM_STYLE,
        get_widget_state(ctx, id, (flags & DISABLED) == 0,
            data.key_state == 1))*/"item",
        layout_spec, flags, id);
    if ((flags & DISABLED) == 0)
    {
        clicked_ = detect_click(ctx, id, LEFT_BUTTON) ||
            detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
        //if (ctx.event->type == RENDER_EVENT &&
        //    detect_potential_click(ctx, id) ||
        //    detect_click_in_progress(ctx, id, LEFT_BUTTON))
        //{
        //    ctx.surface->set_mouse_cursor(HAND_CURSOR);
        //}
        if (id_has_focus(ctx, id) && (flags & NO_FOCUS_BORDER) == 0)
        {
            ctx.artist->draw_focus_rect(data.focus_rect_data,
                panel_.get_region());
        }
        add_to_focus_order(ctx, id);
    }
    else
        clicked_ = false;
}

}
