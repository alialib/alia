#include <alia/link_panel.hpp>
#include <alia/context.hpp>
#include <alia/input_utils.hpp>
#include <alia/surface.hpp>

namespace alia {

struct link_panel_data
{
    link_panel_data() : key_state(0) {}
    int key_state;
};

void link_panel::begin(context& ctx, unsigned flags,
    layout const& layout_spec, region_id id)
{
    if (!id) id = get_region_id(ctx);
    link_panel_data& data = *get_data<link_panel_data>(ctx);
    panel_.begin(ctx, ctx.artist->get_code_for_style(ITEM_STYLE,
        get_widget_state(ctx, id, (flags & DISABLED) == 0,
            data.key_state == 1)),
        flags, layout_spec, id);
    if ((flags & DISABLED) == 0)
    {
        clicked_ = detect_click(ctx, id, LEFT_BUTTON) ||
            detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
        if (ctx.event->type == RENDER_EVENT &&
            detect_potential_click(ctx, id) ||
            detect_click_in_progress(ctx, id, LEFT_BUTTON))
        {
            ctx.surface->set_mouse_cursor(HAND_CURSOR);
        }
        add_to_focus_order(ctx, id);
    }
    else
        clicked_ = false;
}

}
