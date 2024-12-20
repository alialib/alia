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

interaction_status
get_interaction_status(
    dataless_ui_context ctx, widget_id id, interaction_status overrides)
{
    interaction_status status = NO_FLAGS;
    if (is_disabled(overrides))
    {
        status = WIDGET_DISABLED;
    }
    else
    {
        if (is_click_in_progress(ctx, id, mouse_button::LEFT)
            || is_active(overrides))
        {
            status = WIDGET_ACTIVE;
        }
        else if (is_click_possible(ctx, id))
        {
            status = WIDGET_HOVERED;
        }
        if (widget_has_focus(ctx, id) && get_system(ctx).input.window_has_focus
            && get_system(ctx).input.keyboard_interaction)
        {
            status = status | WIDGET_FOCUSED;
        }
    }
    return status;
}

} // namespace alia
