#include <alia/abi/ui/input/keyboard.h>

#include <optional>

#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/events.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/state.h>
#include <alia/abi/ui/system/input_processing.h>
#include <alia/impl/events.hpp>
#include <alia/ui/system/internal_api.h>

using namespace alia;
using namespace alia::operators;

extern "C" {

void
alia_input_acknowledge_key_event(alia_context* ctx)
{
    ALIA_ASSERT(
        get_event_type(*ctx) == ALIA_EVENT_KEY_PRESS
        || get_event_type(*ctx) == ALIA_EVENT_KEY_RELEASE
        || get_event_type(*ctx) == ALIA_EVENT_GLOBAL_KEY_PRESS
        || get_event_type(*ctx) == ALIA_EVENT_GLOBAL_KEY_RELEASE);
    using Payload = alia_key_input;
    static_assert(
        std::is_same_v<decltype(as_key_press_event(*ctx)), Payload&>
        && std::is_same_v<decltype(as_key_release_event(*ctx)), Payload&>
        && std::is_same_v<decltype(as_global_key_press_event(*ctx)), Payload&>
        && std::
            is_same_v<decltype(as_global_key_release_event(*ctx)), Payload&>);
    Payload& payload = unsafe_get_event_payload<Payload>(*ctx);
    payload.acknowledged = true;
}

void
alia_element_add_to_focus_order(alia_context* ctx, alia_element_id id)
{
    if (get_event_type(*ctx) == ALIA_EVENT_FOCUS_SUCCESSOR)
    {
        auto& event = as_focus_successor_event(*ctx);
        if (event.just_saw_target)
        {
            event.successor = id;
            event.just_saw_target = false;
        }
        if (alia_element_id_equal(id, event.target))
        {
            event.just_saw_target = true;
        }
    }
    else if (get_event_type(*ctx) == ALIA_EVENT_FOCUS_PREDECESSOR)
    {
        auto& event = as_focus_predecessor_event(*ctx);
        if (alia_element_id_equal(id, event.target)
            && alia_element_id_is_valid(event.predecessor))
        {
            event.saw_target = true;
        }
        if (!event.saw_target)
        {
            event.predecessor = id;
        }
    }
}

bool
alia_element_has_focus(alia_context* ctx, alia_element_id id)
{
    return alia_element_id_equal(ctx->input->element_with_focus, id);
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
        // TODO: set_focus(*ctx->system, make_routable_element_id(*ctx, id));
    }
}

bool
alia_element_detect_key_press(
    alia_context* ctx, alia_element_id id, alia_key_info* out)
{
    alia_element_focus_on_click(ctx, id);
    // TODO: The event should be targeted to the element.
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_PRESS
        && alia_element_has_focus(ctx, id))
    {
        auto const& event = as_key_press_event(*ctx);
        if (!event.acknowledged)
        {
            *out = event.key;
            return true;
        }
    }
    return false;
}

bool
alia_element_detect_key_release(
    alia_context* ctx, alia_element_id id, alia_key_info* out)
{
    alia_element_focus_on_click(ctx, id);
    // TODO: The event should be targeted to the element.
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_RELEASE
        && alia_element_has_focus(ctx, id))
    {
        auto const& event = as_key_release_event(*ctx);
        if (!event.acknowledged)
        {
            *out = event.key;
            return true;
        }
    }
    return false;
}

bool
alia_input_detect_key_press(alia_context* ctx, alia_key_info* out)
{
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_PRESS)
    {
        auto const& event = as_key_press_event(*ctx);
        if (!event.acknowledged)
        {
            *out = event.key;
            return true;
        }
    }
    return false;
}

bool
alia_input_detect_key_release(alia_context* ctx, alia_key_info* out)
{
    if (get_event_type(*ctx) == ALIA_EVENT_KEY_RELEASE)
    {
        auto const& event = as_key_release_event(*ctx);
        if (!event.acknowledged)
        {
            *out = event.key;
            return true;
        }
    }
    return false;
}

bool
alia_input_detect_global_key_press(alia_context* ctx, alia_key_info* out)
{
    if (get_event_type(*ctx) == ALIA_EVENT_GLOBAL_KEY_PRESS)
    {
        auto const& event = as_global_key_press_event(*ctx);
        if (!event.acknowledged)
        {
            *out = event.key;
            return true;
        }
    }
    return false;
}

bool
alia_input_detect_global_key_release(alia_context* ctx, alia_key_info* out)
{
    if (get_event_type(*ctx) == ALIA_EVENT_GLOBAL_KEY_RELEASE)
    {
        auto const& event = as_global_key_release_event(*ctx);
        if (!event.acknowledged)
        {
            *out = event.key;
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
    alia_key_info spec)
{
    alia_key_info incoming;
    if (alia_element_detect_key_press(ctx, id, &incoming))
    {
        if (alia_key_info_matches_spec(incoming, spec))
        {
            if (state->state == 0)
                state->state = 1;
            alia_input_acknowledge_key_event(ctx);
        }
    }
    else if (alia_element_detect_key_release(ctx, id, &incoming))
    {
        if (alia_key_info_matches_spec(incoming, spec))
        {
            bool const proper = state->state == 1;
            state->state = 0;
            return proper;
        }
    }
    else if (alia_element_detect_focus_loss(ctx, id))
    {
        state->state = 0;
    }
    return false;
}

} // extern "C"
