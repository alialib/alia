#ifndef ALIA_UI_UTILITIES_KEYBOARD_HPP
#define ALIA_UI_UTILITIES_KEYBOARD_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>

// This file provides various utilities for working with keyboard input.

namespace alia {

// Add the given element to the list that will be traversed when the user
// presses Tab.
void
add_to_focus_order(dataless_ui_context ctx, internal_element_id element);

// Determine if the given element has the keyboard focus.
bool
element_has_focus(ui_system& sys, internal_element_id element);

// Determine if the given element has the keyboard focus.
inline bool
element_has_focus(dataless_ui_context ctx, internal_element_id element)
{
    return element_has_focus(get_system(ctx), element);
}

// Set the component with focus and ensure that it's visible.
void
set_focus(ui_system& sys, external_element_id element);

// Detect if a component has just gained focus.
bool
detect_focus_gain(dataless_ui_context ctx, internal_element_id element);

// Detect if a component has just lost focus.
bool
detect_focus_loss(dataless_ui_context ctx, internal_element_id element);

// Calling this ensure that a element will steal the focus if it's click on.
void
focus_on_click(dataless_ui_context ctx, internal_element_id element);

// The following are used to detect general keyboard events related to
// any key...

// Detect if a key press just occurred and was directed at the given element.
std::optional<modded_key>
detect_key_press(dataless_ui_context ctx, internal_element_id element);
// same, but without ID (as background)
std::optional<modded_key>
detect_key_press(dataless_ui_context ctx);

// Detect if a key release just occurred and was directed at the given element.
// Note that many key presses may be received before the corresponding (single)
// key release is received.
std::optional<modded_key>
detect_key_release(dataless_ui_context ctx, internal_element_id element);
// same, but without ID (as background)
// bool
// detect_key_release(dataless_ui_context ctx, modded_key* info);

// Detect text input (as UTF-8 text) directed to a component.
std::optional<std::string>
detect_text_input(dataless_ui_context ctx, internal_element_id element);
// same, but without ID (as background)
// bool
// detect_text_input(dataless_ui_context ctx, std::string* text);

// If you use any of the above detect_ functions, you need to call this
// if you actually process the event.
void
acknowledge_key_event(dataless_ui_context ctx);

// The following are used to detect specific keys...
// The acknowledgement is done automatically.

// Detect if the given key (plus optional modifiers) was just pressed.
bool
detect_key_press(
    dataless_ui_context ctx,
    internal_element_id element,
    key_code code,
    key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
// bool
// detect_key_press(
//     dataless_ui_context ctx, key_code code, key_modifiers modifiers =
//     KMOD_NONE);

// Detect if the given key (plus optional modifiers) was just released.
bool
detect_key_release(
    dataless_ui_context ctx,
    internal_element_id element,
    key_code code,
    key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool
detect_key_release(
    dataless_ui_context ctx,
    key_code code,
    key_modifiers modifiers = KMOD_NONE);

// A keyboard click is a keyboard interface to a UI button that operates in a
// similar manner to the mouse interface. Instead of triggering immediately
// when the key is pressed, the button will be pressed down and will trigger
// when the key is released.
struct keyboard_click_state
{
    keyboard_click_state() : state(0)
    {
    }
    int state;
};
inline bool
is_pressed(keyboard_click_state const& state)
{
    return state.state == 1;
}
// Detect a keyboard click.
bool
detect_keyboard_click(
    dataless_ui_context ctx,
    keyboard_click_state& state,
    internal_element_id element,
    key_code code = key_code::SPACE,
    key_modifiers modifiers = KMOD_NONE);

} // namespace alia

#endif
