#include <alia/ui/utilities/miscellany.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

routable_widget_id
make_routable_widget_id(dataless_ui_context& ctx, widget_id id)
{
    return routable_widget_id(id, get_active_region(ctx.routing));
}

widget_id get_widget_id(ui_context& ctx)
{
    widget_identity* id;
    get_cached_data(ctx, &id);
    return id;
}

widget_state
get_widget_state(
    dataless_ui_context& ctx, widget_id id, widget_state overrides)
{
    widget_state state;
    if (!(overrides & WIDGET_DISABLED))
    {
        if (overrides & WIDGET_SELECTED)
        {
            state = WIDGET_SELECTED;
        }
        else if (is_click_in_progress(ctx, id, LEFT_BUTTON) ||
            (overrides & WIDGET_DEPRESSED))
        {
            state = WIDGET_DEPRESSED;
        }
        else if (is_click_possible(ctx, id))
        {
            state = WIDGET_HOT;
        }
        else
        {
            state = WIDGET_NORMAL;
        }
        if (id_has_focus(ctx, id) && ctx.system->input.window_has_focus &&
            ctx.system->input.keyboard_interaction)
        {
            state = state | WIDGET_FOCUSED;
        }
    }
    else
        state = WIDGET_DISABLED;
    return state;
}

widget_state get_button_state(
    dataless_ui_context& ctx, widget_id id, button_input_state const& state)
{
    return get_widget_state(ctx, id,
        is_pressed(state.key) ? WIDGET_DEPRESSED : NO_FLAGS);
}

bool do_button_input(
    dataless_ui_context& ctx, widget_id id, button_input_state& state)
{
    add_to_focus_order(ctx, id);
    return detect_click(ctx, id, LEFT_BUTTON) ||
        detect_keyboard_click(ctx, state.key, id, KEY_SPACE);
}

void set_active_overlay(dataless_ui_context& ctx, widget_id id)
{
    // Only set the active overlay if none is currently set
    if(ctx.system->overlay_id.id == null_widget_id.id)
        ctx.system->overlay_id = make_routable_widget_id(ctx, id);
}

void clear_active_overlay(dataless_ui_context& ctx)
{ ctx.system->overlay_id = null_widget_id; }

void record_content_change(dataless_ui_context& ctx)
{
    ui_caching_node* cacher = ctx.active_cacher;
    while (cacher)
    {
        ++cacher->layout_valid = false;
        cacher = cacher->parent;
    }
}

}
