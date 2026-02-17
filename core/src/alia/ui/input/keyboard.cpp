#include <alia/abi/ui/input/keyboard.h>

#include <optional>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/state.h>
#include <alia/events.hpp>
#include <alia/ids.hpp>
#include <alia/system/api.hpp>
#include <alia/system/input_processing.hpp>

using namespace alia;

extern "C" {

void
alia_input_acknowledge_key_event(alia_context* ctx)
{
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_PRESS)
        as_key_press_event(*ctx).acknowledged = true;
}

void
alia_element_add_to_focus_order(alia_context* ctx, alia_element_id id)
{
    if (get_event_type(*ctx) == ALIA_EVENT_FOCUS_SUCCESSOR)
    {
        auto& event = as_focus_successor_event(*ctx);
        if (event.just_saw_target)
        {
            event.successor = make_routable_element_id(*ctx, id);
            event.just_saw_target = false;
        }
        if (id == event.target)
        {
            event.just_saw_target = true;
        }
    }
    else if (get_event_type(*ctx) == ALIA_EVENT_FOCUS_PREDECESSOR)
    {
        auto& event = as_focus_predecessor_event(*ctx);
        if (id == event.target
            && alia_routable_element_id_is_valid(event.predecessor))
        {
            event.saw_target = true;
        }
        if (!event.saw_target)
        {
            event.predecessor = make_routable_element_id(*ctx, id);
        }
    }
}

bool
alia_element_has_focus(alia_context* ctx, alia_element_id id)
{
    return alia_routable_element_id_matches(
        ctx->input->element_with_focus, id);
}

bool
alia_element_detect_focus_gain(alia_context* ctx, alia_element_id id)
{
    return get_event_type(*ctx) == ALIA_EVENT_FOCUS_GAIN
        && as_focus_gain_event(*ctx).target == id;
}

bool
alia_element_detect_focus_loss(alia_context* ctx, alia_element_id id)
{
    return get_event_type(*ctx) == ALIA_EVENT_FOCUS_LOSS
        && as_focus_loss_event(*ctx).target == id;
}

void
alia_element_focus_on_click(alia_context* ctx, alia_element_id id)
{
    if (alia_element_is_hovered(ctx, id)
        && (get_event_type(*ctx) == ALIA_EVENT_MOUSE_PRESS
            || get_event_type(*ctx) == ALIA_EVENT_DOUBLE_CLICK))
    {
        set_focus(*ctx->system, make_routable_element_id(*ctx, id));
    }
}

bool
alia_element_detect_key_press(
    alia_context* ctx, alia_element_id id, alia_modded_key* out)
{
    alia_element_focus_on_click(ctx, id);
    // TODO: The event should be targeted to the element.
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_PRESS
        && alia_element_has_focus(ctx, id))
    {
        auto const& event = as_key_press_event(*ctx);
        if (!event.acknowledged)
        {
            *out = {event.code, event.mods};
            return true;
        }
    }
    return false;
}

bool
alia_element_detect_key_release(
    alia_context* ctx, alia_element_id id, alia_modded_key* out)
{
    alia_element_focus_on_click(ctx, id);
    // TODO: The event should be targeted to the element.
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_RELEASE
        && alia_element_has_focus(ctx, id))
    {
        auto const& event = as_key_release_event(*ctx);
        if (!event.acknowledged)
        {
            *out = {event.code, event.mods};
            return true;
        }
    }
    return false;
}

bool
alia_input_detect_key_press(alia_context* ctx, alia_modded_key* out)
{
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_PRESS)
    {
        auto const& event = as_key_press_event(*ctx);
        if (!event.acknowledged)
        {
            *out = {event.code, event.mods};
            return true;
        }
    }
    return false;
}

bool
alia_input_detect_key_release(alia_context* ctx, alia_modded_key* out)
{
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_RELEASE)
    {
        auto const& event = as_key_release_event(*ctx);
        if (!event.acknowledged)
        {
            *out = {event.code, event.mods};
            return true;
        }
    }
    return false;
}

bool
alia_element_detect_keyboard_click(
    alia_context* ctx,
    alia_keyboard_click_state* state,
    alia_element_id id,
    alia_key_code_t code,
    alia_kmods_t mods)
{
    // TODO: This is broken somehow.
    // auto key = detect_key_press(ctx, id);
    // if (key)
    // {
    //     if (key->code == code && key->mods == modifiers)
    //     {
    //         if (state.state == 0)
    //             state.state = 1;
    //         acknowledge_key_event(ctx);
    //     }
    //     // This behavior doesn't really feel right, but it was apparently
    //     here
    //     // for a reason.
    //     // else if (state.state == 1)
    //     //     state.state = 2;
    // }
    // else if (detect_key_release(ctx, id, code, modifiers))
    // {
    //     bool proper = state.state == 1;
    //     state.state = 0;
    //     return proper;
    // }
    // else if (detect_focus_loss(ctx, id))
    // {
    //     state.state = 0;
    // }
    return false;
}

} // extern "C"
