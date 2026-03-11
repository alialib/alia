#include <alia/abi/ui/input/pointer.h>

#include <alia/abi/events.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/style.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/ids.h>
#include <alia/ui/system/internal_api.h>

using namespace alia::operators;
using namespace alia;

extern "C" {

alia_vec2f
alia_input_pointer_position(alia_context* ctx)
{
    return ctx->input->mouse_position - ctx->geometry->offset;
}

bool
alia_input_pointer_in_box(alia_context* ctx, alia_box const* box)
{
    return alia_input_pointer_in_surface(ctx)
        && alia_box_contains(*box, alia_input_pointer_position(ctx))
        && alia_box_contains(
               alia_geometry_get_clip_region(ctx), ctx->input->mouse_position);
}

static bool
detect_mouse_press(alia_context* ctx, alia_button_t button)
{
    return (get_event_type(*ctx) == ALIA_EVENT_MOUSE_PRESS
            && as_mouse_press_event(*ctx).button == button)
        || (get_event_type(*ctx) == ALIA_EVENT_DOUBLE_CLICK
            && as_double_click_event(*ctx).button == button);
}

bool
alia_element_detect_mouse_press(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    if (!alia_element_is_hovered(ctx, id) || !detect_mouse_press(ctx, button))
        return false;
    set_element_with_capture(*ctx->system, make_routable_element_id(*ctx, id));
    return true;
}

bool
alia_element_detect_mouse_release(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    return get_event_type(*ctx) == ALIA_EVENT_MOUSE_RELEASE
        && as_mouse_release_event(*ctx).button == button
        && alia_element_has_capture(ctx, id);
}

bool
alia_element_detect_mouse_motion(alia_context* ctx, alia_element_id id)
{
    return get_event_type(*ctx) == ALIA_EVENT_MOUSE_MOTION
        // TODO: Revisit this logic. What if the element itself has capture?
        && alia_no_element_has_capture(ctx)
        && alia_element_is_hovered(ctx, id);
}

bool
alia_element_detect_double_click(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    return get_event_type(*ctx) == ALIA_EVENT_DOUBLE_CLICK
        && as_double_click_event(*ctx).button == button
        && alia_element_is_hovered(ctx, id);
}

bool
alia_element_detect_click(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    (void) alia_element_detect_mouse_press(ctx, id, button);
    return alia_element_detect_mouse_release(ctx, id, button)
        && alia_element_is_hovered(ctx, id);
}

bool
alia_element_is_click_possible(alia_context* ctx, alia_element_id id)
{
    return alia_element_is_hovered(ctx, id)
        && alia_no_element_has_capture(ctx);
}

bool
alia_element_is_click_in_progress(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    return alia_element_is_hovered(ctx, id)
        && alia_element_has_capture(ctx, id)
        && alia_input_button_is_down(ctx, button);
}

bool
alia_element_detect_drag(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    (void) alia_element_detect_mouse_press(ctx, id, button);
    return get_event_type(*ctx) == ALIA_EVENT_MOUSE_MOTION
        && alia_input_button_is_down(ctx, button)
        && alia_element_has_capture(ctx, id);
}

bool
alia_element_detect_press_or_drag(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    bool press = alia_element_detect_mouse_press(ctx, id, button);
    bool motion = get_event_type(*ctx) == ALIA_EVENT_MOUSE_MOTION
               && alia_input_button_is_down(ctx, button);
    return (press || motion) && alia_element_has_capture(ctx, id);
}

alia_vec2f
alia_element_get_mouse_motion_delta(alia_context* ctx, alia_element_id id)
{
    // TODO: Implement this.
    alia_vec2f zero = {0.0f, 0.0f};
    return zero;
    // mouse_motion_event* event;
    // if (!detect_event(ctx, &event))
    //     return make_vector<double>(0, 0);
    // matrix<3, 3, double> matrix = inverse(get_transformation(ctx));
    // // The system's `mouse_position` field is updated immediately after the
    // // mouse_motion_event is delivered, so by subtracting that from the one
    // // carried by the event, we get the delta.
    // return transform(matrix, vector<2, double>(event->position))
    //      - transform(
    //            matrix,
    //            vector<2, double>(get_system(ctx).input.mouse_position));
}

bool
alia_element_is_drag_in_progress(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    return alia_input_button_is_down(ctx, button)
        && alia_element_has_capture(ctx, id) && ctx->input->dragging;
}

bool
alia_element_detect_drag_release(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    return alia_element_is_drag_in_progress(ctx, id, button)
        && alia_element_detect_mouse_release(ctx, id, button);
}

bool
alia_element_detect_stationary_click(
    alia_context* ctx, alia_element_id id, alia_button_t button)
{
    return alia_element_detect_click(ctx, id, button) && !ctx->input->dragging;
}

bool
alia_element_detect_mouse_gain(alia_context* ctx, alia_element_id id)
{
    return get_event_type(*ctx) == ALIA_EVENT_MOUSE_GAIN
        && get_event_target(*ctx) == id;
}

bool
alia_element_detect_mouse_loss(alia_context* ctx, alia_element_id id)
{
    return get_event_type(*ctx) == ALIA_EVENT_MOUSE_LOSS
        && get_event_target(*ctx) == id;
}

bool
alia_element_detect_scroll(
    alia_context* ctx, alia_element_id id, alia_vec2f* out_delta)
{
    if (get_event_type(*ctx) == ALIA_EVENT_WHEEL
        && get_event_target(*ctx) == id)
    {
        *out_delta = as_wheel_event(*ctx).delta;
        return true;
    }
    return false;
}

} // extern "C"
