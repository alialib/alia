#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/system.hpp>

namespace alia {

void acknowledge_input_event(dataless_ui_context& ctx)
{
    get_event<input_event>(ctx).acknowledged = true;
}

bool id_has_focus(dataless_ui_context& ctx, widget_id id)
{
    return ctx.system->input.focused_id.id == id;
}

bool detect_focus_gain(dataless_ui_context& ctx, widget_id id)
{
    if (detect_event(ctx, FOCUS_GAIN_EVENT))
    {
        focus_notification_event& e = get_event<focus_notification_event>(ctx);
        if (e.target == id)
            return true;
    }
    return false;
}

bool detect_focus_loss(dataless_ui_context& ctx, widget_id id)
{
    if (detect_event(ctx, FOCUS_LOSS_EVENT))
    {
        focus_notification_event& e = get_event<focus_notification_event>(ctx);
        if (e.target == id)
            return true;
    }
    return false;
}

void add_to_focus_order(dataless_ui_context& ctx, widget_id id)
{
    switch (ctx.event->type)
    {
     case FOCUS_PREDECESSOR_EVENT:
      {
        focus_predecessor_event& e = get_event<focus_predecessor_event>(ctx);
        if (e.input_id == id && is_valid(e.predecessor))
            e.saw_input = true;
        if (!e.saw_input)
            e.predecessor = make_routable_widget_id(ctx, id);
        break;
      }
     case FOCUS_SUCCESSOR_EVENT:
      {
        focus_successor_event& e = get_event<focus_successor_event>(ctx);
        if (e.just_saw_input)
        {
            e.successor = make_routable_widget_id(ctx, id);
            e.just_saw_input = false;
        }
        if (e.input_id == id)
            e.just_saw_input = true;
        break;
      }
     //case FOCUS_RECOVERY_EVENT:
     // {
     //   focus_recovery_event& e = get_event<focus_recovery_event>(ctx);
     //   if ((id->focus_bits & 2) != 0)
     //       e.id = id;
     //   break;
     // }
    }
}

void set_focus(ui_system& ui, routable_widget_id id)
{
    bool different = ui.input.focused_id.id != id.id;
    if (different && is_valid(ui.input.focused_id))
    {
        // A lot of code likes to call set_focus in response to events, which
        // means that the following FOCUS_LOSS_EVENT could end up being invoked
        // on a UI state that hasn't seen a refresh event yet, so do a refresh
        // here just to be safe.
        refresh_ui(ui);

        focus_notification_event e(FOCUS_LOSS_EVENT, ui.input.focused_id.id);
        issue_targeted_event(ui, e, ui.input.focused_id);
    }

    ui.input.focused_id = id;

    // It's possible to have widgets that appear based on whether or not
    // another widget has the focus, so we need to refresh here.
    refresh_ui(ui);

    if (different && is_valid(id))
    {
        widget_visibility_request request;
        request.widget = id;
        request.move_to_top = false;
        request.abrupt = false;
        ui.pending_visibility_requests.push_back(request);

        focus_notification_event e(FOCUS_GAIN_EVENT, id.id);
        issue_targeted_event(ui, e, id);
    }
}

// Calling this ensure that a widget will steal the focus if it's click on.
static void do_click_focus(dataless_ui_context& ctx, widget_id id)
{
    if (detect_event(ctx, MOUSE_PRESS_EVENT) && is_region_hot(ctx, id))
    {
        set_focus(ctx, id);
        end_pass(ctx);
    }
}

static bool detect_key_event(
    dataless_ui_context& ctx, key_event_info* info, ui_event_type event_type)
{
    if (detect_event(ctx, event_type))
    {
        key_event& e = get_event<key_event>(ctx);
        if (!e.acknowledged)
        {
            *info = e.info;
            return true;
        }
    }
    return false;
}

bool detect_key_press(
    dataless_ui_context& ctx, key_event_info* info, widget_id id)
{
    do_click_focus(ctx, id);
    return id_has_focus(ctx, id) &&
        detect_key_event(ctx, info, KEY_PRESS_EVENT);
}
bool detect_key_press(dataless_ui_context& ctx, key_event_info* info)
{
    return detect_key_event(ctx, info, BACKGROUND_KEY_PRESS_EVENT);
}

bool detect_key_release(
    dataless_ui_context& ctx, key_event_info* info, widget_id id)
{
    do_click_focus(ctx, id);
    return id_has_focus(ctx, id) &&
        detect_key_event(ctx, info, KEY_RELEASE_EVENT);
}
bool detect_key_release(dataless_ui_context& ctx, key_event_info* info)
{
    return detect_key_event(ctx, info, BACKGROUND_KEY_RELEASE_EVENT);
}

static bool detect_text_input(
    dataless_ui_context& ctx, utf8_string* text, ui_event_type event_type)
{
    if (detect_event(ctx, event_type))
    {
        text_input_event& e = get_event<text_input_event>(ctx);
        if (!e.acknowledged)
        {
            *text = e.text;
            return true;
        }
    }
    return false;
}

bool detect_text_input(
    dataless_ui_context& ctx, utf8_string* text, widget_id id)
{
    do_click_focus(ctx, id);
    return id_has_focus(ctx, id) &&
        detect_text_input(ctx, text, TEXT_INPUT_EVENT);
}
bool detect_text_input(dataless_ui_context& ctx, utf8_string* text)
{
    return detect_text_input(ctx, text, BACKGROUND_TEXT_INPUT_EVENT);
}

bool detect_key_press(dataless_ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info, id) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_press(
    dataless_ui_context& ctx, key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_release(dataless_ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_release(ctx, &info, id) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_release(dataless_ui_context& ctx, key_code code,
    key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_release(ctx, &info) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

void set_focus(dataless_ui_context& ctx, widget_id id)
{
    set_focus(*ctx.system, make_routable_widget_id(ctx, id));
}

bool detect_keyboard_click(
    dataless_ui_context& ctx, keyboard_click_state& state,
    widget_id id, key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info, id))
    {
        if (info.code == code && info.mods == modifiers)
        {
            if (state.state == 0)
                state.state = 1;
            acknowledge_input_event(ctx);
        }
        else if (state.state == 1)
            state.state = 2;
    }
    else if (detect_key_release(ctx, id, code, modifiers))
    {
        bool proper = state.state == 1;
        state.state = 0;
        return proper;
    }
    return false;
}
bool detect_keyboard_click(
    dataless_ui_context& ctx, keyboard_click_state& state,
    key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info))
    {
        if (info.code == code && info.mods == modifiers)
        {
            if (state.state == 0)
                state.state = 1;
            acknowledge_input_event(ctx);
        }
        else if (state.state == 1)
            state.state = 2;
    }
    else if (detect_key_release(ctx, code, modifiers))
    {
        bool proper = state.state == 1;
        state.state = 0;
        return proper;
    }
    return false;
}

}
