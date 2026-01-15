#pragma once

#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/geometry.hpp>
#include <alia/input/constants.hpp>
#include <alia/system/object.hpp>

namespace alia {

// TODO: Move elsewhere.
inline affine2
get_transformation(ephemeral_context& ctx)
{
    // TODO
    return identity_matrix();
}

// TODO: Move elsewhere.
inline box
get_clip_region(ephemeral_context& ctx)
{
    // TODO
    return {.min = {0, 0}, .size = get_system(ctx).surface_size};
}

// Get the mouse position in the current frame of reference.
vec2f
get_mouse_position(ephemeral_context& ctx);

// Detect if the element with the given ID is under the mouse.
bool
is_element_hot(ui_system& sys, alia_element_id id);

// same, but operating on a context
inline bool
is_element_hot(ephemeral_context& ctx, alia_element_id id)
{
    return is_element_hot(*ctx.system, id);
}

// Detect if the given ID has captured the mouse, meaning that a mouse button
// was pressed down when the mouse was over the element with the given ID, and
// the mouse button is still down.
bool
element_has_capture(ui_system& sys, alia_element_id id);
// same, but operating on a context
inline bool
element_has_capture(ephemeral_context& ctx, alia_element_id id)
{
    return element_has_capture(*ctx.system, id);
}

bool
no_element_has_capture(ui_system& sys);

inline bool
no_element_has_capture(ephemeral_context& ctx)
{
    return no_element_has_capture(*ctx.system);
}

bool
is_mouse_in_surface(ui_system& sys);

inline bool
is_mouse_in_surface(ephemeral_context& ctx)
{
    return is_mouse_in_surface(*ctx.system);
}

bool
is_mouse_button_pressed(ui_system& sys, button button);

inline bool
is_mouse_button_pressed(ephemeral_context& ctx, button button)
{
    return is_mouse_button_pressed(*ctx.system, button);
}

bool
detect_mouse_press(ephemeral_context& ctx, button button);

bool
detect_mouse_press(ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_mouse_release(ephemeral_context& ctx, button button);

bool
detect_mouse_release(
    ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_mouse_motion(ephemeral_context& ctx, alia_element_id id);

bool
detect_double_click(ephemeral_context& ctx, button button);

bool
detect_double_click(ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_click(ephemeral_context& ctx, alia_element_id id, button button);

bool
is_click_possible(ui_system& sys, alia_element_id id);

inline bool
is_click_possible(ephemeral_context& ctx, alia_element_id id)
{
    return is_click_possible(*ctx.system, id);
}

bool
is_click_in_progress(ui_system& sys, alia_element_id id, button button);

inline bool
is_click_in_progress(ephemeral_context& ctx, alia_element_id id, button button)
{
    return is_click_in_progress(*ctx.system, id, button);
}

bool
detect_drag(ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_press_or_drag(
    ephemeral_context& ctx, alia_element_id id, button button);

vec2f
get_mouse_motion_delta(ephemeral_context& ctx, alia_element_id id);

bool
is_drag_in_progress(ui_system& sys, alia_element_id id, button button);

inline bool
is_drag_in_progress(ephemeral_context& ctx, alia_element_id id, button button)
{
    return is_drag_in_progress(*ctx.system, id, button);
}

bool
detect_drag_release(ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_scroll(ephemeral_context& ctx, alia_element_id id, vec2f* out_delta);

inline nanosecond_count
get_click_start_time(ui_system& sys, button button)
{
    return sys.input.last_mouse_press_time[unsigned(button)];
}

inline nanosecond_count
get_click_start_time(ephemeral_context& ctx, button button)
{
    return get_click_start_time(*ctx.system, button);
}

// TODO: Implement this.
// inline nanosecond_count
// get_click_duration(
//     ephemeral_context& ctx,
//     mouse_button button,
//     nanosecond_count max_duration)
// {
//     return max_duration
//          - get_raw_animation_ticks_left(
//                ctx, get_click_start_time(ctx, button) + max_duration);
// }

} // namespace alia
