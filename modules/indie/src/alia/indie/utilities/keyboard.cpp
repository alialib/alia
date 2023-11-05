#include "alia/indie/events/delivery.hpp"
#include "alia/indie/utilities/mouse.hpp"
#include <alia/indie/utilities/keyboard.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/node_interface.hpp>
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
add_to_focus_order(event_context ctx, internal_element_ref element)
{
    {
        focus_successor_event* event;
        if (detect_event(ctx, &event))
        {
            if (event->just_saw_target)
            {
                event->successor = element;
                event->just_saw_target = false;
            }
            if (element == event->target)
            {
                event->just_saw_target = true;
            }
        }
    }
    {
        focus_predecessor_event* event;
        if (detect_event(ctx, &event))
        {
            if (element == event->target && event->predecessor)
            {
                event->saw_target = true;
            }
            if (!event->saw_target)
            {
                event->predecessor = element;
            }
        }
    }
}

bool
element_has_focus(system& sys, internal_element_ref element)
{
    return sys.input.element_with_focus.matches(element);
}

void
set_focus(system& sys, external_element_ref element)
{
    // TODO: Some of this logic seems to be asking for alia to have an internal
    // event queue.

    bool different = sys.input.element_with_focus != element;
    if (different && sys.input.element_with_focus)
    {
        // A lot of code likes to call set_focus in response to events, which
        // means that the following FOCUS_LOSS_EVENT could end up being invoked
        // on a UI state that hasn't seen a refresh event yet, so do a refresh
        // here just to be safe.
        refresh_system(sys);

        focus_notification_event event{{{}, input_event_type::FOCUS_LOSS}};
        deliver_input_event(sys, sys.input.element_with_focus.widget, event);
    }

    sys.input.element_with_focus = element;

    // It's possible to have widgets that appear based on whether or not
    // another widget has the focus, so we need to refresh here.
    refresh_system(sys);

    if (different && element)
    {
        // TODO: Make the new widget visible.
        // widget_visibility_request request;
        // request.widget = id;
        // request.move_to_top = false;
        // request.abrupt = false;
        // ui.pending_visibility_requests.push_back(request);

        focus_notification_event event{{{}, input_event_type::FOCUS_GAIN}};
        deliver_input_event(sys, element.widget, event);
    }
}

void
focus_on_click(event_context ctx, internal_element_ref element)
{
    mouse_button_event* event;
    if (is_element_hot(ctx, element) && detect_event(ctx, &event)
        && (event->type == input_event_type::MOUSE_PRESS
            || event->type == input_event_type::DOUBLE_CLICK))
    {
        set_focus(get_system(ctx), externalize(element));
        // TODO: end_pass(ctx);
    }
}

bool
detect_focus_gain(event_context ctx, internal_element_ref)
{
    focus_notification_event* event;
    return detect_event(ctx, &event)
           && event->type == input_event_type::FOCUS_GAIN;
}

bool
detect_focus_loss(event_context ctx, internal_element_ref)
{
    focus_notification_event* event;
    return detect_event(ctx, &event)
           && event->type == input_event_type::FOCUS_LOSS;
}

std::optional<modded_key>
detect_key_press(event_context ctx, internal_element_ref element)
{
    focus_on_click(ctx, element);

    key_event* event;
    if (element_has_focus(ctx, element) && detect_event(ctx, &event)
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
    internal_element_ref element,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_press(ctx, element);
    if (key && key->code == code && key->mods == modifiers)
    {
        acknowledge_key_event(ctx);
        return true;
    }
    return false;
}

std::optional<modded_key>
detect_key_release(event_context ctx, internal_element_ref element)
{
    focus_on_click(ctx, element);

    key_event* event;
    if (element_has_focus(ctx, element) && detect_event(ctx, &event)
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
    internal_element_ref element,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_release(ctx, element);
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
    internal_element_ref element,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_press(ctx, element);
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
    else if (detect_key_release(ctx, element, code, modifiers))
    {
        bool proper = state.state == 1;
        state.state = 0;
        return proper;
    }
    else if (detect_focus_loss(ctx, element))
    {
        state.state = 0;
    }
    return false;
}

}} // namespace alia::indie
