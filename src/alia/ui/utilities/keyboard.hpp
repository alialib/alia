#ifndef ALIA_UI_UTILITIES_KEYBOARD_HPP
#define ALIA_UI_UTILITIES_KEYBOARD_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with keyboard input.

namespace alia {

// Add the given widget ID to the list of widgets that will be traversed
// when the user presses Tab.
void add_to_focus_order(dataless_ui_context& ctx, widget_id id);

// Determine if the given widget ID has the keyboard focus.
bool id_has_focus(dataless_ui_context& ctx, widget_id id);

// Set the widget with focus and ensure that it's visible.
void set_focus(dataless_ui_context& ctx, widget_id id);

// Set the widget with focus and ensure that it's visible.
void set_focus(ui_system& ctx, routable_widget_id id);

// Get the ID of the widget before the one with the focus.
widget_id get_id_before_focus(dataless_ui_context& ctx);

// Get the ID of the widget after the one with the focus.
widget_id get_id_after_focus(dataless_ui_context& ctx);

// Detect if the widget has just gained focus.
bool detect_focus_gain(dataless_ui_context& ctx, widget_id id);

// Detect if the widget has just lost focus.
bool detect_focus_loss(dataless_ui_context& ctx, widget_id id);

// The following are used to detect general keyboard events related to
// any key...

// Detect if a key press just occurred and was directed at the given
// widget.  The return value indicates whether or not an event occured.
// If one did, the 'info' argument is filled with the info.
bool detect_key_press(
    dataless_ui_context& ctx, key_event_info* info, widget_id id);
// same, but without ID (as background)
bool detect_key_press(dataless_ui_context& ctx, key_event_info* info);

// Detect if a key release just occurred and was directed at the given
// widget.  Note that many key presses may be received before the
// corresponding (single) key release is received.
bool detect_key_release(
    dataless_ui_context& ctx, key_event_info* info, widget_id id);
// same, but without ID (as background)
bool detect_key_release(dataless_ui_context& ctx, key_event_info* info);

// Detect a normal ASCII character key press, as seen by the given widget.
bool detect_text_input(
    dataless_ui_context& ctx, utf8_string* text, widget_id id);
// same, but without ID (as background)
bool detect_text_input(dataless_ui_context& ctx, utf8_string* text);

// If you use any of the above detect_ functions, you need to call this
// if you actually process the event.
void acknowledge_input_event(dataless_ui_context& ctx);

// The following are used to detect specific keys...
// The acknowledgement is done automatically.

// Detect if the given key (plus optional modifiers) was just pressed.
bool detect_key_press(dataless_ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool detect_key_press(dataless_ui_context& ctx, key_code code,
    key_modifiers modifiers = KMOD_NONE);

// Detect if the given key (plus optional modifiers) was just released.
bool detect_key_release(dataless_ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool detect_key_release(dataless_ui_context& ctx, key_code code,
    key_modifiers modifiers = KMOD_NONE);

// A keyboard click is a keyboard interface to a UI button that operates in a
// similar manner to the mouse interface. Instead of triggering immediately
// when the key is pressed, the button will be pressed down and will trigger
// when the key is released.
struct keyboard_click_state
{
    keyboard_click_state() : state(0) {}
    int state;
};
static inline bool is_pressed(keyboard_click_state const& state)
{ return state.state == 1; }
// Detect a keyboard click.
bool detect_keyboard_click(
    dataless_ui_context& ctx, keyboard_click_state& state,
    widget_id id, key_code code = KEY_SPACE,
    key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool detect_keyboard_click(
    dataless_ui_context& ctx, keyboard_click_state& state,
    key_code code = KEY_SPACE, key_modifiers modifiers = KMOD_NONE);

}

#endif
