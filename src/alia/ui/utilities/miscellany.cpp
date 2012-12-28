#include <alia/ui/utilities/miscellany.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

routable_widget_id make_routable_widget_id(ui_context& ctx, widget_id id)
{
    return routable_widget_id(id, get_active_region(ctx.routing));
}

widget_id get_widget_id(ui_context& ctx)
{
    widget_identity* id;
    get_data(ctx, &id);
    return id;
}

widget_state get_widget_state(
    ui_context& ctx, widget_id id, bool enabled, bool pressed, bool selected)
{
    widget_state state;
    if (enabled)
    {
        if (selected)
            state = WIDGET_SELECTED;
        else if (detect_click_in_progress(ctx, id, LEFT_BUTTON) || pressed)
            state = WIDGET_DEPRESSED;
        else if (detect_potential_click(ctx, id))
            state = WIDGET_HOT;
        else
            state = WIDGET_NORMAL;
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

widget_state get_button_state(ui_context& ctx, widget_id id,
    button_input_state const& state)
{
    return get_widget_state(ctx, id, true, is_pressed(state.key));
}

bool do_button_input(ui_context& ctx, widget_id id, button_input_state& state)
{
    add_to_focus_order(ctx, id);
    return detect_click(ctx, id, LEFT_BUTTON) ||
        detect_keyboard_click(ctx, state.key, id, KEY_SPACE);
}

}
