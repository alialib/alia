#pragma once

#include <alia/context.h>
#include <alia/geometry.h>
#include <alia/ids.h>
#include <alia/input/constants.h>
#include <alia/system.h>

#ifdef __cplusplus
extern "C" {
#endif

// Get the pointer position in the current frame of reference.
alia_vec2f
alia_pointer_position(alia_ephemeral_context* ctx);

// Detect if the element with the given ID is under the pointer.
bool
alia_element_is_hovered(alia_ui_system* sys, alia_element_id id)
{
    return sys->input.hot_element.element == id;
}

bool
alia_element_has_capture(alia_ui_system* sys, alia_element_id id)
{
    return sys->input.element_with_capture.element == id;
}

bool
alia_no_element_has_capture(alia_ui_system* sys)
{
    return !alia_element_id_is_valid(sys.input.element_with_capture.element);
}

// Detect if the given ID has captured the mouse, meaning that a mouse button
// was pressed down when the mouse was over the element with the given ID, and
// the mouse button is still down.
bool
alia_element_has_capture(alia_ui_system* sys, alia_element_id id);

bool
alia_no_element_has_capture(alia_ui_system* sys);

bool
alia_surface_is_hovered(alia_ui_system* sys);

bool
alia_mouse_button_is_pressed(alia_ui_system* sys, alia_button_t button);

bool
detect_mouse_press(ephemeral_context& ctx, mouse_button button);

bool
detect_mouse_press(
    ephemeral_context& ctx, alia_element_id id, mouse_button button);

bool
detect_mouse_release(ephemeral_context& ctx, mouse_button button);

bool
detect_mouse_release(
    ephemeral_context& ctx, alia_element_id id, mouse_button button);

bool
detect_mouse_motion(ephemeral_context& ctx, alia_element_id id);

bool
detect_double_click(ephemeral_context& ctx, mouse_button button);

bool
detect_double_click(
    ephemeral_context& ctx, alia_element_id id, mouse_button button);

bool
detect_click(ephemeral_context& ctx, alia_element_id id, mouse_button button);

bool
is_click_possible(ui_system& sys, alia_element_id id);

inline bool
is_click_possible(ephemeral_context& ctx, alia_element_id id)
{
    return is_click_possible(*ctx.system, id);
}

bool
is_click_in_progress(ui_system& sys, alia_element_id id, mouse_button button);

inline bool
is_click_in_progress(
    ephemeral_context& ctx, alia_element_id id, mouse_button button)
{
    return is_click_in_progress(*ctx.system, id, button);
}

bool
detect_drag(ephemeral_context& ctx, alia_element_id id, mouse_button button);

bool
detect_press_or_drag(
    ephemeral_context& ctx, alia_element_id id, mouse_button button);

vec2f
get_mouse_motion_delta(ephemeral_context& ctx, alia_element_id id);

bool
is_drag_in_progress(ui_system& sys, alia_element_id id, mouse_button button);

inline bool
is_drag_in_progress(
    ephemeral_context& ctx, alia_element_id id, mouse_button button)
{
    return is_drag_in_progress(*ctx.system, id, button);
}

bool
detect_drag_release(
    ephemeral_context& ctx, alia_element_id id, mouse_button button);

bool
detect_scroll(ephemeral_context& ctx, alia_element_id id, vec2f* out_delta);

inline nanosecond_count
get_click_start_time(ui_system& sys, mouse_button button)
{
    return sys.input.last_mouse_press_time[unsigned(button)];
}

inline nanosecond_count
get_click_start_time(ephemeral_context& ctx, mouse_button button)
{
    return get_click_start_time(*ctx.system, button);
}

#ifdef __cplusplus
}
#endif
