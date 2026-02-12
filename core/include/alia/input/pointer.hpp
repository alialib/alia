#pragma once

#include <alia/abi/base/geometry.h>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/input/constants.hpp>
#include <alia/system/object.hpp>

namespace alia {

// Get the mouse position in the current frame of reference.
alia_vec2f
get_mouse_position(ephemeral_context& ctx);

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
is_click_possible(ephemeral_context& ctx, alia_element_id id);

bool
is_click_in_progress(
    ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_drag(ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_press_or_drag(
    ephemeral_context& ctx, alia_element_id id, button button);

alia_vec2f
get_mouse_motion_delta(ephemeral_context& ctx, alia_element_id id);

bool
is_drag_in_progress(ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_drag_release(ephemeral_context& ctx, alia_element_id id, button button);

bool
detect_scroll(
    ephemeral_context& ctx, alia_element_id id, alia_vec2f* out_delta);

inline alia_nanosecond_count
get_click_start_time(ui_system& sys, button button)
{
    return sys.input.last_mouse_press_time[unsigned(button)];
}

inline alia_nanosecond_count
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
