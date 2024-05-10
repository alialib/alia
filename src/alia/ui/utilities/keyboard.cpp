#include <alia/ui/utilities/keyboard.hpp>

#include <optional>

#include <alia/core/flow/events.hpp>
#include <alia/core/flow/top_level.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/mouse.hpp>

namespace alia {

void
acknowledge_key_event(dataless_ui_context ctx)
{
    key_event* event;
    if (detect_event(ctx, &event))
        event->acknowledged = true;
}

void
add_to_focus_order(dataless_ui_context ctx, widget_id id)
{
    {
        focus_successor_event* event;
        if (detect_event(ctx, &event))
        {
            if (event->just_saw_target)
            {
                event->successor = make_routable_widget_id(ctx, id);
                event->just_saw_target = false;
            }
            if (id == event->target)
            {
                event->just_saw_target = true;
            }
        }
    }
    {
        focus_predecessor_event* event;
        if (detect_event(ctx, &event))
        {
            if (id == event->target && event->predecessor)
            {
                event->saw_target = true;
            }
            if (!event->saw_target)
            {
                event->predecessor = make_routable_widget_id(ctx, id);
            }
        }
    }
}

bool
element_has_focus(ui_system& sys, widget_id id)
{
    return sys.input.widget_with_focus.matches(id);
}

void
set_focus(ui_system& sys, routable_widget_id widget)
{
    // TODO: Some of this logic seems to be asking for alia to have an internal
    // event queue.

    bool different = sys.input.widget_with_focus.id != widget.id;
    if (different && sys.input.widget_with_focus)
    {
        // A lot of code likes to call set_focus in response to events, which
        // means that the following FOCUS_LOSS_EVENT could end up being invoked
        // on a UI state that hasn't seen a refresh event yet, so do a refresh
        // here just to be safe.
        refresh_system(sys);

        focus_notification_event event;
        dispatch_targeted_event(
            sys, event, sys.input.widget_with_focus, FOCUS_LOSS_EVENT);
        refresh_system(sys);
    }

    sys.input.widget_with_focus = widget;

    // It's possible to have widgets that appear based on whether or not
    // another widget has the focus, so we need to refresh here.
    refresh_system(sys);

    // TODO
    // if (different && element)
    // {
    //     // Make the new widget visible.
    //     {
    //         auto widget = element.widget.lock();
    //         if (widget && widget->parent)
    //         {
    //             widget->parent->reveal_region(region_reveal_request{
    //                 layout_box(transform_box(
    //                     widget->transformation(),
    //                     box2d(widget->bounding_box()))),
    //                 false,
    //                 false});
    //         }
    //     }

    //     focus_notification_event event{{{}, ui_event_type::FOCUS_GAIN}};
    //     deliver_input_event(sys, element.widget, event);
    // }
}

void
focus_on_click(dataless_ui_context ctx, widget_id id)
{
    if (is_element_hot(ctx, id)
        && (get_event_type(ctx) == MOUSE_PRESS_EVENT
            || get_event_type(ctx) == DOUBLE_CLICK_EVENT))
    {
        set_focus(get_system(ctx), make_routable_widget_id(ctx, id));
    }
}

bool
detect_focus_gain(dataless_ui_context ctx, widget_id id)
{
    return get_event_type(ctx) == FOCUS_GAIN_EVENT
           && cast_event<focus_notification_event>(ctx).target == id;
}

bool
detect_focus_loss(dataless_ui_context ctx, widget_id id)
{
    return get_event_type(ctx) == FOCUS_LOSS_EVENT
           && cast_event<focus_notification_event>(ctx).target == id;
}

std::optional<modded_key>
detect_key_press(dataless_ui_context ctx, widget_id id)
{
    focus_on_click(ctx, id);

    if (get_event_type(ctx) == KEY_PRESS_EVENT && element_has_focus(ctx, id))
    {
        auto const& event = cast_event<key_event>(ctx);
        if (!event.acknowledged)
            return event.key;
    }
    return std::nullopt;
}

std::optional<modded_key>
detect_key_press(dataless_ui_context ctx)
{
    key_event* event;
    if (detect_event(ctx, &event)
        && get_event_type(ctx) == BACKGROUND_KEY_PRESS_EVENT)
    {
        if (!event->acknowledged)
            return event->key;
    }
    return std::nullopt;
}

bool
detect_key_press(
    dataless_ui_context ctx,
    widget_id id,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_press(ctx, id);
    if (key && key->code == code && key->mods == modifiers)
    {
        acknowledge_key_event(ctx);
        return true;
    }
    return false;
}

std::optional<modded_key>
detect_key_release(dataless_ui_context ctx, widget_id id)
{
    focus_on_click(ctx, id);

    if (element_has_focus(ctx, id) && get_event_type(ctx) == KEY_RELEASE_EVENT)
    {
        auto const& event = cast_event<key_event>(ctx);
        if (!event.acknowledged)
            return event.key;
    }
    return std::nullopt;
}

bool
detect_key_release(
    dataless_ui_context ctx,
    widget_id id,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_release(ctx, id);
    if (key && key->code == code && key->mods == modifiers)
    {
        acknowledge_key_event(ctx);
        return true;
    }
    return false;
}

bool
detect_keyboard_click(
    dataless_ui_context ctx,
    keyboard_click_state& state,
    widget_id id,
    key_code code,
    key_modifiers modifiers)
{
    auto key = detect_key_press(ctx, id);
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
    else if (detect_key_release(ctx, id, code, modifiers))
    {
        bool proper = state.state == 1;
        state.state = 0;
        return proper;
    }
    else if (detect_focus_loss(ctx, id))
    {
        state.state = 0;
    }
    return false;
}

} // namespace alia
