#ifndef ALIA_UI_UTILITIES_MOUSE_HPP
#define ALIA_UI_UTILITIES_MOUSE_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with mouse input.

namespace alia {

// Get the mouse position in the current frame of reference.
vector<2,double> get_mouse_position(ui_context& ctx);
// Same, but rounded to integer coordinates.
vector<2,int> get_integer_mouse_position(ui_context& ctx);

// Is the mouse cursor within the surface?
bool is_mouse_in_surface(ui_context& ctx);

// Check if the given mouse button is pressed.
bool is_mouse_button_pressed(ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been pressed.
bool detect_mouse_press(ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been pressed over the given region.
bool detect_mouse_press(ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has just been released.
bool detect_mouse_release(ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been released over the given region.
bool detect_mouse_release(ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has just been double-clicked over the given
// region.
bool detect_double_click(ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has been pressed and released over the given
// region.
bool detect_click(ui_context& ctx, widget_id id, mouse_button button);

// Detect if the mouse is over the given area and the mouse could
// potentially be clicked on that area.  Unlike is_region_hot(), this
// returns false if the mouse is currently captured by something else.
bool detect_potential_click(ui_context& ctx, widget_id id);

// Detect if a mouse button is currently down over an region and was
// originally pressed down over that same region.
bool detect_click_in_progress(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect if the mouse is being dragged with the given mouse button down.
bool detect_drag(ui_context& ctx, widget_id id, mouse_button button);

// Detect if the mouse is pressed or dragged within the given region with the
// given mouse button down.
bool detect_mouse_down(ui_context& ctx, widget_id id, mouse_button button);

// If the current event is a drag, this will return the mouse movement
// represented by this event, in the current frame of reference.
vector<2,double> get_drag_delta(ui_context& ctx);

bool detect_drag_in_progress(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect if the mouse has just been released after a drag.
bool detect_drag_release(ui_context& ctx, widget_id id, mouse_button button);

bool detect_mouse_motion(ui_context& ctx, widget_id id);

// Detect if a mouse button has been pressed and released over the given
// region without 'significant' movement during the click.
bool detect_stationary_click(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect if the mouse is being dragged with the given mouse button down,
// but only after there has been 'sigificant' movement since the click.
bool detect_explicit_drag(ui_context& ctx, widget_id id, mouse_button button);

// Detect if the mouse has just been released after a drag.
bool detect_explicit_drag_release(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect scroll wheel movement.
// The return value is positive for upward movement.
bool detect_wheel_movement(ui_context& ctx, float* movement, widget_id id);

}

#endif
