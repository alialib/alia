#ifndef ALIA_UI_UTILITIES_MOUSE_HPP
#define ALIA_UI_UTILITIES_MOUSE_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/geometry.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/system/object.hpp>

namespace alia {

// TODO: Move elsewhere.
inline matrix<3, 3, double> const&
get_transformation(dataless_ui_context ctx)
{
    return get_geometry_context(ctx).transformation_matrix;
}

// Get the mouse position in the current frame of reference.
vector<2, double>
get_mouse_position(dataless_ui_context ctx);
// same, but rounded to integer coordinates
vector<2, int>
get_integer_mouse_position(dataless_ui_context ctx);

// Detect if the widget with the given ID is under the mouse.
bool
is_widget_hot(ui_system& sys, widget_id id);
// same, but operating on a context
inline bool
is_widget_hot(dataless_ui_context ctx, widget_id id)
{
    return is_widget_hot(get_system(ctx), id);
}

// Detect if the given ID has captured the mouse, meaning that a mouse
// button was pressed down when the mouse was over the widget with the
// given ID, and the mouse button is still down.
bool
widget_has_capture(ui_system& sys, widget_id id);
// same, but operating on a context
inline bool
widget_has_capture(dataless_ui_context ctx, widget_id id)
{
    return widget_has_capture(get_system(ctx), id);
}

bool
no_widget_has_capture(ui_system& sys);

inline bool
no_widget_has_capture(dataless_ui_context ctx)
{
    return no_widget_has_capture(get_system(ctx));
}

bool
is_mouse_in_surface(ui_system& sys);

inline bool
is_mouse_in_surface(dataless_ui_context ctx)
{
    return is_mouse_in_surface(get_system(ctx));
}

bool
is_mouse_button_pressed(ui_system& sys, mouse_button button);

inline bool
is_mouse_button_pressed(dataless_ui_context ctx, mouse_button button)
{
    return is_mouse_button_pressed(get_system(ctx), button);
}

bool
detect_mouse_press(dataless_ui_context ctx, mouse_button button);

bool
detect_mouse_press(dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_mouse_release(dataless_ui_context ctx, mouse_button button);

bool
detect_mouse_release(
    dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_mouse_motion(dataless_ui_context ctx, widget_id id);

bool
detect_double_click(dataless_ui_context ctx, mouse_button button);

bool
detect_double_click(
    dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_click(dataless_ui_context ctx, widget_id id, mouse_button button);

bool
is_click_possible(ui_system& sys, widget_id id);

inline bool
is_click_possible(dataless_ui_context ctx, widget_id id)
{
    return is_click_possible(get_system(ctx), id);
}

bool
is_click_in_progress(ui_system& sys, widget_id id, mouse_button button);

inline bool
is_click_in_progress(
    dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return is_click_in_progress(get_system(ctx), id, button);
}

bool
detect_drag(dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_press_or_drag(
    dataless_ui_context ctx, widget_id id, mouse_button button);

vector<2, double>
get_mouse_motion_delta(dataless_ui_context ctx, widget const& widget);

bool
is_drag_in_progress(ui_system& sys, widget_id id, mouse_button button);

inline bool
is_drag_in_progress(dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return is_drag_in_progress(get_system(ctx), id, button);
}

bool
detect_drag_release(
    dataless_ui_context ctx, widget_id id, mouse_button button);

std::optional<vector<2, double>>
detect_scroll(dataless_ui_context ctx, widget_id id);

inline millisecond_count
get_click_start_time(ui_system& sys, mouse_button button)
{
    return sys.input.last_mouse_press_time[unsigned(button)];
}

inline millisecond_count
get_click_start_time(dataless_ui_context ctx, mouse_button button)
{
    return get_click_start_time(get_system(ctx), button);
}

inline millisecond_count
get_click_duration(
    dataless_ui_context ctx,
    mouse_button button,
    millisecond_count max_duration)
{
    return max_duration
           - get_raw_animation_ticks_left(
               ctx, get_click_start_time(ctx, button) + max_duration);
}

} // namespace alia

#endif
