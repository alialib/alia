#include "alia/indie/utilities/mouse.hpp"
#include <alia/indie/utilities/keyboard.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/internals.hpp>
#include <alia/indie/system/api.hpp>

namespace alia { namespace indie {

void
add_to_focus_order(event_context ctx, widget const*)
{
    focus_query_event* event;
    if (detect_event(ctx, &event))
        event->widget_wants_focus = true;
}

bool
widget_has_focus(system& sys, widget const* widget)
{
    return sys.input.widget_with_focus.matches(widget);
}

void
set_focus(system& sys, external_widget_handle widget)
{
    sys.input.widget_with_focus = widget;
}

void
focus_on_click(event_context ctx, widget const* widget)
{
    mouse_button_event* event;
    if (is_widget_hot(ctx, widget) && detect_event(ctx, &event)
        && (event->type == input_event_type::MOUSE_PRESS
            || event->type == input_event_type::DOUBLE_CLICK))
    {
        set_focus(get_system(ctx), externalize(widget));
        // TODO: end_pass(ctx);
    }
}

}} // namespace alia::indie
