#ifndef ALIA_ABI_UI_INPUT_POINTER_H
#define ALIA_ABI_UI_INPUT_POINTER_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/kernel/timing.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/input/state.h>

ALIA_EXTERN_C_BEGIN

// Get the pointer position in the current frame of reference.
alia_vec2f
alia_input_pointer_position(alia_ui_context* ctx);

static inline bool
alia_input_pointer_in_surface(alia_ui_context* ctx)
{
    return alia_ctx_input(ctx)->mouse_inside_window;
}

bool
alia_input_pointer_in_box(alia_ui_context* ctx, alia_box const* box);

static inline bool
alia_input_button_is_down(alia_ui_context* ctx, alia_button_t button)
{
    return (alia_ctx_input(ctx)->mouse_button_state
            & (1u << (unsigned) button))
        != 0;
}

inline alia_nanosecond_count
alia_input_click_start_time(alia_ui_context* ctx, alia_button_t button)
{
    return alia_ctx_input(ctx)->last_mouse_press_time[unsigned(button)];
}

static inline alia_nanosecond_count
alia_input_click_duration(alia_ui_context* ctx, alia_button_t button)
{
    return alia_timing_tick_count(alia_ctx_kernel(ctx))
         - alia_input_click_start_time(ctx, button);
}

// Detect if the element with the given ID is under the pointer.
static inline bool
alia_element_is_hovered(alia_ui_context* ctx, alia_element_id id)
{
    return alia_element_id_equal(alia_ctx_input(ctx)->hot_element, id);
}

static inline bool
alia_no_element_has_capture(alia_ui_context* ctx)
{
    return !alia_element_id_is_valid(
        alia_ctx_input(ctx)->element_with_capture);
}

// Detect if an element has captured the mouse. - Capture is implicitly
// acquired when a mouse button is pressed down while the mouse is over an
// element, and it stays acquired as long as the mouse button is held down.
static inline bool
alia_element_has_capture(alia_ui_context* ctx, alia_element_id id)
{
    return alia_element_id_equal(
        alia_ctx_input(ctx)->element_with_capture, id);
}

bool
alia_element_detect_mouse_press(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_mouse_release(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_mouse_motion(alia_ui_context* ctx, alia_element_id id);

bool
alia_element_detect_double_click(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_click(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_is_click_possible(alia_ui_context* ctx, alia_element_id id);

bool
alia_element_is_click_in_progress(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_drag(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_press_or_drag(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

alia_vec2f
alia_element_get_mouse_motion_delta(alia_ui_context* ctx, alia_element_id id);

bool
alia_element_is_drag_in_progress(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_drag_release(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_stationary_click(
    alia_ui_context* ctx, alia_element_id id, alia_button_t button);

bool
alia_element_detect_mouse_gain(alia_ui_context* ctx, alia_element_id id);

bool
alia_element_detect_mouse_loss(alia_ui_context* ctx, alia_element_id id);

bool
alia_element_detect_scroll(
    alia_ui_context* ctx, alia_element_id id, alia_vec2f* out_delta);

// why the system chose the draw event's cursor target
typedef uint8_t alia_cursor_reason_t;

enum
{
    ALIA_CURSOR_REASON_NONE = 0,
    // pointer is over the element (no capture)
    ALIA_CURSOR_REASON_HOT = 1,
    // element has pointer capture
    ALIA_CURSOR_REASON_CAPTURE = 2,
};

// Active DRAW event payload, or NULL when the current event is not DRAW.
static inline alia_draw*
alia_ctx_active_draw(alia_ui_context* ctx)
{
    alia_event* event = alia_ctx_events(alia_ctx_kernel(ctx))->event;
    if (!event || event->type != ALIA_EVENT_DRAW)
        return NULL;
    return (alia_draw*) event->payload;
}

// Determine if the active draw event is asking for a cursor for `id`.
static inline bool
alia_element_is_cursor_target(alia_ui_context* ctx, alia_element_id id)
{
    alia_draw const* draw = alia_ctx_active_draw(ctx);
    if (!draw)
        return false;
    return alia_element_id_equal(draw->cursor_target, id);
}

// Return the reason the current cursor target was chosen, or NONE outside
// drawing / when there is no target.
static inline alia_cursor_reason_t
alia_element_cursor_reason(alia_ui_context* ctx)
{
    alia_draw const* draw = alia_ctx_active_draw(ctx);
    if (!draw)
        return ALIA_CURSOR_REASON_NONE;
    return draw->cursor_reason;
}

// Set the draw event's cursor when `id` is the cursor target. Later calls
// win, so wrappers can override after invoking a child.
static inline void
alia_element_set_cursor(
    alia_ui_context* ctx, alia_element_id id, alia_cursor_t cursor)
{
    alia_draw* draw = alia_ctx_active_draw(ctx);
    if (!draw)
        return;
    if (!alia_element_id_equal(draw->cursor_target, id))
        return;
    draw->cursor = cursor;
}

ALIA_EXTERN_C_END

#endif // ALIA_UI_INPUT_POINTER_H
