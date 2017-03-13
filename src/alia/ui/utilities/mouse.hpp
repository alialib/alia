#ifndef ALIA_UI_UTILITIES_MOUSE_HPP
#define ALIA_UI_UTILITIES_MOUSE_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with mouse input.

// All functions that start with the detect_ prefix check for particular
// events. They can only return true on passes that correspond to the
// delivery of that particular event.
// In contrast, functions with the is_ prefix are simply polling state that
// is always available with the UI system. They can return true on any pass.

namespace alia {

// Get the mouse position in the current frame of reference.
vector<2,double> get_mouse_position(dataless_ui_context& ctx);
// Same, but rounded to integer coordinates.
vector<2,int> get_integer_mouse_position(dataless_ui_context& ctx);

// Is the mouse cursor within the surface?
bool is_mouse_in_surface(dataless_ui_context& ctx);

// Check if the given mouse button is pressed.
bool is_mouse_button_pressed(dataless_ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been pressed.
bool detect_mouse_press(dataless_ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been pressed over the given region.
bool detect_mouse_press(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has just been released.
bool detect_mouse_release(dataless_ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been released over the given region.
bool detect_mouse_release(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has just been double-clicked over the given
// region.
bool detect_double_click(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has been pressed and released over the given
// region.
bool detect_click(dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect if the mouse is over the given region and the mouse could
// potentially be clicked on that region. Unlike is_region_hot(), this
// returns false if the mouse is currently captured by something else.
bool is_click_possible(dataless_ui_context& ctx, widget_id id);

// Detect if a mouse button is currently down over a region and was
// originally pressed over that same region.
bool is_click_in_progress(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect drags over the region with thg given ID and involving the given
// mouse button. A drag is defined as moving the mouse while holding down a
// button.
bool detect_drag(dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect if the given mouse button is pressed or dragged over the given
// region.
// This is equivalent to the following.
// detect_mouse_press(ctx, id, button) || detect_drag(ctx, id, button)
bool detect_press_or_drag(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// If the current event is a drag, this will return the mouse movement
// represented by this event, in the current frame of reference.
vector<2,double> get_drag_delta(dataless_ui_context& ctx);

// Is the mouse currently being dragged over the region with the given ID?
// (with the given button held down)
bool is_drag_in_progress(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect if the mouse has just been released after a drag.
bool detect_drag_release(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect any mouse motion over the given region.
bool detect_mouse_motion(dataless_ui_context& ctx, widget_id id);

// Detect if a mouse button has been pressed and released over the given
// region without any mouse movement during the click.
bool detect_stationary_click(
    dataless_ui_context& ctx, widget_id id, mouse_button button);

// Detect scroll wheel movement.
// The return value is positive for upward movement.
bool detect_wheel_movement(
    dataless_ui_context& ctx, float* movement, widget_id id);

// Detect if the mouse has just entered a region.
bool detect_mouse_gain(dataless_ui_context& ctx, widget_id id);

// Detect if the mouse has just left a region.
bool detect_mouse_loss(dataless_ui_context& ctx, widget_id id);

}

#endif
