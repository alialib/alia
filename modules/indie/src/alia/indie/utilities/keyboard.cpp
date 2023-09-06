#include "alia/indie/events/delivery.hpp"
#include "alia/indie/utilities/mouse.hpp"
#include <alia/indie/utilities/keyboard.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/internals.hpp>
#include <alia/indie/system/api.hpp>
#include <optional>

namespace alia { namespace indie {

void
acknowledge_key_event(event_context ctx)
{
    key_event* event;
    if (detect_event(ctx, &event))
        event->acknowledged = true;
}

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
    // TODO: Some of this logic seems to be asking for alia to have an internal
    // event queue.

    bool different = sys.input.widget_with_focus != widget;
    if (different && sys.input.widget_with_focus)
    {
        // A lot of code likes to call set_focus in response to events, which
        // means that the following FOCUS_LOSS_EVENT could end up being invoked
        // on a UI state that hasn't seen a refresh event yet, so do a refresh
        // here just to be safe.
        refresh_system(sys);

        focus_notification_event event{{{}, input_event_type::FOCUS_LOSS}};
        deliver_input_event(sys, sys.input.widget_with_focus, event);
    }

    sys.input.widget_with_focus = widget;

    // It's possible to have widgets that appear based on whether or not
    // another widget has the focus, so we need to refresh here.
    refresh_system(sys);

    if (different && widget)
    {
        // TODO: Make the new widget visible.
        // widget_visibility_request request;
        // request.widget = id;
        // request.move_to_top = false;
        // request.abrupt = false;
        // ui.pending_visibility_requests.push_back(request);

        focus_notification_event event{{{}, input_event_type::FOCUS_GAIN}};
        deliver_input_event(sys, widget, event);
    }
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

bool
detect_focus_gain(event_context ctx, widget const*)
{
    focus_notification_event* event;
    return detect_event(ctx, &event)
           && event->type == input_event_type::FOCUS_GAIN;
}

bool
detect_focus_loss(event_context ctx, widget const*)
{
    focus_notification_event* event;
    return detect_event(ctx, &event)
           && event->type == input_event_type::FOCUS_LOSS;
}

std::optional<modded_key>
detect_key_press(event_context ctx, widget const* widget)
{
    focus_on_click(ctx, widget);

    key_event* event;
    if (widget_has_focus(ctx, widget) && detect_event(ctx, &event)
        && event->type == input_event_type::KEY_PRESS)
    {
        if (!event->acknowledged)
            return event->key;
    }
    return std::nullopt;
}

bool
detect_key_press(
    event_context ctx,
    widget const* widget,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_press(ctx, widget);
    if (key && key->code == code && key->mods == modifiers)
    {
        acknowledge_key_event(ctx);
        return true;
    }
    return false;
}

std::optional<modded_key>
detect_key_release(event_context ctx, widget const* widget)
{
    focus_on_click(ctx, widget);

    key_event* event;
    if (widget_has_focus(ctx, widget) && detect_event(ctx, &event)
        && event->type == input_event_type::KEY_RELEASE)
    {
        if (!event->acknowledged)
            return event->key;
    }
    return std::nullopt;
}

bool
detect_key_release(
    event_context ctx,
    widget const* widget,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_release(ctx, widget);
    if (key && key->code == code && key->mods == modifiers)
    {
        acknowledge_key_event(ctx);
        return true;
    }
    return false;
}

bool
detect_keyboard_click(
    event_context ctx,
    keyboard_click_state& state,
    widget const* widget,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_press(ctx, widget);
    if (key)
    {
        if (key->code == code && key->mods == modifiers)
        {
            if (state.state == 0)
                state.state = 1;
            acknowledge_key_event(ctx);
        }
        // This behavior doesn't really feel right, but it was apparently here
        // for a reason.
        // else if (state.state == 1)
        //     state.state = 2;
    }
    else if (detect_key_release(ctx, widget, code, modifiers))
    {
        bool proper = state.state == 1;
        state.state = 0;
        return proper;
    }
    else if (detect_focus_loss(ctx, widget))
    {
        state.state = 0;
    }
    return false;
}

}} // namespace alia::indie
