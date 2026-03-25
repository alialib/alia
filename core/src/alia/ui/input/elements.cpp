#include <alia/abi/ui/input/elements.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/input/pointer.h>

using namespace alia;
using namespace alia::operators;

extern "C" {

alia_interaction_status_t
alia_element_get_interaction_status(
    alia_context* ctx, alia_element_id id, alia_interaction_status_t overrides)
{
    alia_interaction_status_t status = 0;

    if (overrides & ALIA_INTERACTION_STATUS_DISABLED)
    {
        status = ALIA_INTERACTION_STATUS_DISABLED;
    }
    else if (
        alia_element_is_click_in_progress(ctx, id, ALIA_BUTTON_LEFT)
        || (overrides & ALIA_INTERACTION_STATUS_ACTIVE))
    {
        status = ALIA_INTERACTION_STATUS_ACTIVE;
    }
    else if (alia_element_is_hovered(ctx, id))
    {
        status = ALIA_INTERACTION_STATUS_HOVERED;
    }

    // TODO: Implement focus.
    // if (element_has_focus(ctx, id)
    //     && get_system(ctx).input.window_has_focus
    //     && get_system(ctx).input.keyboard_interaction)
    // {
    //     status = status | ELEMENT_FOCUSED;
    // }

    return status;
}

alia_element_id
alia_element_get_identity(alia_context* ctx)
{
    return alia_make_element_id(ctx, alia_substrate_use_memory(ctx, 1, 1));
}

} // extern "C"
