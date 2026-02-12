#ifndef ALIA_INPUT_API_H
#define ALIA_INPUT_API_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/ids.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_input_state
{
    // Is the mouse inside the window associated with this UI?
    bool mouse_inside_window = false;

    // the state of the mouse buttons (one bit per button)
    unsigned mouse_button_state = 0;

    // the raw mouse position inside the window
    alia_vec2f mouse_position;

    // the tick count corresponding to the last press of each mouse button
    alia_nanosecond_count last_mouse_press_time[ALIA_MAX_SUPPORTED_BUTTONS];

    // the element that the mouse is over
    alia_routable_element_id hot_element;

    // the element that has the mouse captured - Note that this isn't
    // necessarily the same as the hot_element.
    alia_routable_element_id element_with_capture;

    // the element that has the keyboard focus
    alia_routable_element_id element_with_focus;

    // Is the user currently dragging the mouse (with a button pressed)?
    bool dragging = false;

    // Does the window have focus?
    bool window_has_focus = true;

    // Is the user currently interacting with the UI via the keyboard? - This
    // is used as a hint to display focus indicators.
    bool keyboard_interaction = false;

    // If the mouse is hovering over a widget (identified by hot_widget), this
    // is the time at which the hovering started. Note that hovering is only
    // possible if no widget has captured the mouse.
    alia_nanosecond_count hover_start_time;

    // the mouse cursor that's currently set for our window
    alia_cursor_t current_cursor = ALIA_CURSOR_DEFAULT;
} alia_input_state;

// Get the pointer position in the current frame of reference.
alia_vec2f
alia_input_pointer_position(alia_context* ctx);

#if 0

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
is_click_in_progress(
    ephemeral_context& ctx, alia_element_id id, mouse_button button);

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

#endif

ALIA_EXTERN_C_END

#endif // ALIA_INPUT_API_H
