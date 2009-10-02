#ifndef ALIA_INPUT_UTILS_HPP
#define ALIA_INPUT_UTILS_HPP

#include <alia/forward.hpp>
#include <alia/input_defs.hpp>

namespace alia {

// REGIONS...

void refresh_region_id(context& ctx, region_id id);

region_id get_region_id(context& ctx);

void hit_test_region(context& ctx, region_id id, box2i const& region);

void do_region_visibility(context& ctx, region_id id, box2i const& region);

void do_region(context& ctx, region_id id, box2i const& region);

void make_region_visible(context& ctx, region_id id);

bool mouse_is_inside_region(context& ctx, box2i const& region);

// KEYBOARD INPUT...

// The following are used to detect general keyboard events related to
// any key...

// Detect if a key press just occurred and was directed at the given
// widget.  The return value indicates whether or not an event occured.
// If one did, the first argument is filled with the info.
bool detect_key_press(context& ctx, key_event_info* info, region_id id);
// same, but without ID (as background)
bool detect_key_press(context& ctx, key_event_info* info);

// Detect if a key release just occurred and was directed at the given
// widget.  Note that many key presses may be received before the
// corresponding (single) key release is received.
bool detect_key_release(context& ctx, key_event_info* info, region_id id);
// same, but without ID (as background)
bool detect_key_release(context& ctx, key_event_info* info);

// Detect a normal ASCII character key press, as seen by the given widget.
int detect_char(context& ctx, region_id id);
// same, but without ID (as background)
int detect_char(context& ctx);

// If you use any of the above detect_ functions, you need to call this
// if you actually process the event.
void acknowledge_input_event(context& ctx);

// The following are used to detect specific keys... 
// The acknowledgement is done automatically.

// Detect if the given key (plus optional modifiers) was just pressed.
bool detect_key_press(context& ctx, region_id id,
    key_code code, int modifiers = 0);
// same, but without ID (as background)
bool detect_key_press(context& ctx, key_code code, int modifiers = 0);

// Detect if the given key (plus optional modifiers) was just released.
bool detect_key_release(context& ctx, region_id id,
    key_code code, int modifiers = 0);
// same, but without ID (as background)
bool detect_key_release(context& ctx, key_code code, int modifiers = 0);

// Detect if the given key (plus optional modifiers) was just released AND
// it was the last key to be pressed (meaning that there were no other
// keys pressed down while it was being held down).
bool detect_proper_key_release(context& ctx, int& state, region_id id,
    key_code code, int modifiers = 0);
// same, but without ID (as background)
bool detect_proper_key_release(context& ctx, int& state, key_code code,
    int modifiers = 0);

// FOCUS...

void add_to_focus_order(context& ctx, region_id id);

// Determine if the given widget ID has the keyboard focus.
bool id_has_focus(context& ctx, region_id id);

// Set the widget with focus and ensure that it's visible.
void set_focus(context& ctx, region_id id);

// Get the ID of the widget before the one with the focus.
region_id get_id_before_focus(context& ctx);

// Get the ID of the widget after the one with the focus.
region_id get_id_after_focus(context& ctx);

// Call this after the context is initialized to set the focus to the first
// widget in the context.
void set_initial_focus(context& ctx);

bool detect_focus_gain(context& ctx, region_id id);

bool detect_focus_loss(context& ctx, region_id id);

// MOUSE INPUT...

// Is the mouse cursor within the surface?
bool is_mouse_in_surface(context& ctx);

point2i const& get_integer_mouse_position(context& ctx);

// Check if the given mouse button is pressed.
bool is_mouse_button_pressed(context& ctx, mouse_button button);

// Detect if the given ID has captured the mouse, meaning that a mouse
// button was pressed down when the mouse was over the widget with the
// given ID, and the mouse button is still down.
bool is_region_active(context& ctx, region_id id);

// Detect if the mouse is over the given region.
bool is_region_hot(context& ctx, region_id id);

// The following flags can be passed into the detect_ functions...

// Detect if a mouse button has just been pressed.
bool detect_mouse_down(context& ctx, mouse_button button);

// Detect if a mouse button has just been pressed down over the given
// region.
bool detect_mouse_down(context& ctx, region_id id, mouse_button button);

// Detect if a mouse button has just been released.
bool detect_mouse_up(context& ctx, mouse_button button);

// Detect if a mouse button has just been released over the given region.
bool detect_mouse_up(context& ctx, region_id id, mouse_button button);

// Detect if a mouse button has just been double-clicked over the given
// region.
bool detect_double_click(context& ctx, region_id id, mouse_button button);

// Detect if a mouse button has been pressed and released over the given
// region.
bool detect_click(context& ctx, region_id id, mouse_button button);

// Detect if the mouse is over the given area and the mouse could
// potentially be clicked on that area.  Unlike is_region_hot(), this
// returns false if the mouse is currently captured by something else.
bool detect_potential_click(context& ctx, region_id id);

// Detect if a mouse button is currently down over an region and was
// originally pressed down over that same region.
bool detect_click_in_progress(context& ctx, region_id id, mouse_button button);

// Detect if the mouse is being dragged with the given mouse button down.
bool detect_drag(context& ctx, region_id id, mouse_button button);

bool detect_drag_in_progress(context& ctx, region_id id, mouse_button button);

// Detect if the mouse has just been released after a drag.
bool detect_drag_release(context& ctx, region_id id, mouse_button button);

// Detect if a mouse button has been pressed and released over the given
// region without 'significant' movement during the click.
bool detect_stationary_click(context& ctx, region_id id, mouse_button button);

// Detect if the mouse is being dragged with the given mouse button down,
// but only after there has been 'sigificant' movement since the click.
bool detect_explicit_drag(context& ctx, region_id id, mouse_button button);

// Detect if the mouse has just been released after a drag.
bool detect_explicit_drag_release(context& ctx, region_id id,
    mouse_button button);

// Detect scroll wheel movement.
// The return value is positive for upward movement.
int detect_wheel_movement(context& ctx);

}

#endif
