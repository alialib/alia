#include <alia/input/elements.hpp>

#include <alia/input/pointer.hpp>

using namespace alia::operators;

namespace alia {

alia_element_id
get_element_id(ephemeral_context& ctx)
{
    element_identity* id;
    // TODO
    id = nullptr;
    // get_cached_data(ctx, &id);
    return id;
}

interaction_status
get_interaction_status(
    ephemeral_context& ctx, alia_element_id id, interaction_status overrides)
{
    interaction_status status = NO_FLAGS;
    if (is_disabled(overrides))
    {
        status = ELEMENT_DISABLED;
    }
    else
    {
        if (is_click_in_progress(ctx, id, ALIA_BUTTON_LEFT)
            || is_active(overrides))
        {
            status = ELEMENT_ACTIVE;
        }
        else if (is_click_possible(ctx, id))
        {
            status = ELEMENT_HOVERED;
        }
        // TODO: Implement focus.
        // if (element_has_focus(ctx, id)
        //     && get_system(ctx).input.window_has_focus
        //     && get_system(ctx).input.keyboard_interaction)
        // {
        //     status = status | ELEMENT_FOCUSED;
        // }
    }
    return status;
}

} // namespace alia
