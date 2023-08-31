#ifndef ALIA_INDIE_UTILITIES_KEYBOARD_HPP
#define ALIA_INDIE_UTILITIES_KEYBOARD_HPP

#include "alia/indie/widget.hpp"
#include <alia/core/flow/events.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>

// This file provides various utilities for working with keyboard input.

namespace alia { namespace indie {

// Add the given widget to the list of components that will be traversed when
// the user presses Tab.
void
add_to_focus_order(event_context ctx, widget const* widget);

// Determine if the given widget has the keyboard focus.
bool
widget_has_focus(system& sys, widget const* widget);

// Determine if the given widget has the keyboard focus.
inline bool
widget_has_focus(event_context ctx, widget const* widget)
{
    return widget_has_focus(get_system(ctx), widget);
}

// Set the component with focus and ensure that it's visible.
void
set_focus(system& sys, external_widget_handle widget);

// Get the widget before the one with the focus.
component_id
get_widget_before_focus(system& sys);

// Get the widget before the one with the focus.
component_id
get_widget_before_focus(system& sys);

// Detect if a component has just gained focus.
bool
detect_focus_gain(event_context ctx, widget const* widget);

// Detect if a component has just lost focus.
bool
detect_focus_loss(event_context ctx, widget const* widget);

// Calling this ensure that a widget will steal the focus if it's click on.
void
focus_on_click(event_context ctx, widget const* widget);

// The following are used to detect general keyboard events related to
// any key...

// Detect if a key press just occurred and was directed at the given
// widget.  The return value indicates whether or not an event occured.
// If one did, the 'info' argument is filled with the info.
bool
detect_key_press(event_context ctx, modded_key* info, component_id id);
// same, but without ID (as background)
bool
detect_key_press(event_context ctx, modded_key* info);

// Detect if a key release just occurred and was directed at the given
// widget.  Note that many key presses may be received before the
// corresponding (single) key release is received.
bool
detect_key_release(event_context ctx, modded_key* info, component_id id);
// same, but without ID (as background)
bool
detect_key_release(event_context ctx, modded_key* info);

// Detect text input (as UTF-8 text) directed to a component.
bool
detect_text_input(event_context ctx, std::string* text, component_id id);
// same, but without ID (as background)
bool
detect_text_input(event_context ctx, std::string* text);

// If you use any of the above detect_ functions, you need to call this
// if you actually process the event.
void
acknowledge_input_event(event_context ctx);

// The following are used to detect specific keys...
// The acknowledgement is done automatically.

// Detect if the given key (plus optional modifiers) was just pressed.
bool
detect_key_press(
    event_context ctx,
    component_id id,
    key_code code,
    key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool
detect_key_press(
    event_context ctx, key_code code, key_modifiers modifiers = KMOD_NONE);

// Detect if the given key (plus optional modifiers) was just released.
bool
detect_key_release(
    event_context ctx,
    component_id id,
    key_code code,
    key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool
detect_key_release(
    event_context ctx, key_code code, key_modifiers modifiers = KMOD_NONE);

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
static inline bool
is_pressed(keyboard_click_state const& state)
{
    return state.state == 1;
}
// Detect a keyboard click.
bool
detect_keyboard_click(
    event_context ctx,
    keyboard_click_state& state,
    component_id id,
    key_code code = key_code::SPACE,
    key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool
detect_keyboard_click(
    event_context ctx,
    keyboard_click_state& state,
    key_code code = key_code::SPACE,
    key_modifiers modifiers = KMOD_NONE);

}} // namespace alia::indie

#endif
