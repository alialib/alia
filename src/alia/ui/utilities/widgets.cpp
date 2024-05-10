#include <alia/ui/utilities/widgets.hpp>

#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>

namespace alia {

widget_id
get_widget_id(ui_context& ctx)
{
    widget_identity* id;
    get_cached_data(ctx, &id);
    return id;
}

widget_state
get_widget_state(dataless_ui_context ctx, widget_id id, widget_state overrides)
{
    widget_state state;
    if (!(overrides & WIDGET_DISABLED))
    {
        if (overrides & WIDGET_SELECTED)
        {
            state = WIDGET_SELECTED;
        }
        else if (
            is_click_in_progress(ctx, id, mouse_button::LEFT)
            || (overrides & WIDGET_DEPRESSED))
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
        if (widget_has_focus(ctx, id) && get_system(ctx).input.window_has_focus
            && get_system(ctx).input.keyboard_interaction)
        {
            state = state | WIDGET_FOCUSED;
        }
    }
    else
        state = WIDGET_DISABLED;
    return state;
}

} // namespace alia
