#ifndef ALIA_ABI_UI_INPUT_POINTER_H
#define ALIA_ABI_UI_INPUT_POINTER_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/ids.h>
#include <alia/abi/kernel/timing.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/state.h>

ALIA_EXTERN_C_BEGIN

// Get the pointer position in the current frame of reference.
alia_vec2f
alia_input_pointer_position(alia_context* ctx);

static inline bool
alia_input_pointer_in_surface(alia_context* ctx)
{
    return ctx->input->mouse_inside_window;
}

bool
alia_input_pointer_in_box(alia_context* ctx, alia_box const* box);

static inline bool
alia_input_button_is_down(alia_context* ctx, alia_button_t button)
{
    return (ctx->input->mouse_button_state & (1u << (unsigned) button)) != 0;
}

inline alia_nanosecond_count
alia_input_click_start_time(alia_context* ctx, alia_button_t button)
{
    return ctx->input->last_mouse_press_time[unsigned(button)];
}

static inline alia_nanosecond_count
alia_input_click_duration(alia_context* ctx, alia_button_t button)
{
    return alia_timing_tick_count(ctx)
         - alia_input_click_start_time(ctx, button);
}

// Detect if the element with the given ID is under the pointer.
static inline bool
alia_element_is_hovered(alia_context* ctx, alia_element_id id)
{
    return ctx->input->hot_element.element == id;
}

static inline bool
alia_no_element_has_capture(alia_context* ctx)
{
    return !alia_element_id_is_valid(ctx->input->element_with_capture.element);
}

// Detect if an element has captured the mouse. - Capture is implicitly
// acquired when a mouse button is pressed down while the mouse is over an
// element, and it stays acquired as long as the mouse button is held down.
static inline bool
alia_element_has_capture(alia_context* ctx, alia_element_id id)
{
    return ctx->input->element_with_capture.element == id;
}

bool
alia_element_detect_mouse_press(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_mouse_release(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_mouse_motion(alia_context* ctx, alia_element_id id);

bool
alia_element_detect_double_click(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_click(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_is_click_possible(alia_context* ctx, alia_element_id id);

bool
alia_element_is_click_in_progress(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_drag(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_press_or_drag(
    alia_context* ctx, alia_element_id id, alia_button_t button);

alia_vec2f
alia_element_get_mouse_motion_delta(alia_context* ctx, alia_element_id id);

bool
alia_element_is_drag_in_progress(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_drag_release(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_stationary_click(
    alia_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_mouse_gain(alia_context* ctx, alia_element_id id);

bool
alia_element_detect_mouse_loss(alia_context* ctx, alia_element_id id);

bool
alia_element_detect_scroll(
    alia_context* ctx, alia_element_id id, alia_vec2f* out_delta);

ALIA_EXTERN_C_END

#endif // ALIA_UI_INPUT_POINTER_H
