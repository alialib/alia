#include <alia/widget_state.hpp>
#include <alia/input_utils.hpp>

namespace alia {

widget_state get_widget_state(context& ctx, region_id id, bool enabled,
    bool pressed)
{
    widget_state state;
    if (enabled)
    {
        if (detect_click_in_progress(ctx, id, LEFT_BUTTON) || pressed)
        {
            state = widget_states::DEPRESSED;
        }
        else if (detect_potential_click(ctx, id))
        {
            state = widget_states::HOT;
        }
        else
            state = widget_states::NORMAL;

        if (id_has_focus(ctx, id))
            state |= widget_states::FOCUSED;
    }
    else
        state = widget_states::DISABLED;
    return state;
}

}
