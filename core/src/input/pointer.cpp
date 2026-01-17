#include <alia/input/pointer.hpp>

// TODO: API shouldn't be needed here.
#include <alia/system/api.hpp>

using namespace alia::operators;

namespace alia {

vec2f
get_mouse_position(ephemeral_context& ctx)
{
    return alia_affine2_transform_point(
        alia_affine2_invert(get_transformation(ctx)),
        ctx.system->input.mouse_position);
}

bool
is_element_hot(ui_system& sys, alia_element_id id)
{
    return sys.input.hot_element.element == id;
}

bool
element_has_capture(ui_system& sys, alia_element_id id)
{
    return sys.input.element_with_capture.element == id;
}

bool
no_element_has_capture(ui_system& sys)
{
    return !alia_element_id_is_valid(sys.input.element_with_capture.element);
}

bool
is_mouse_in_surface(ui_system& sys)
{
    return sys.input.mouse_inside_window;
}

bool
is_mouse_button_pressed(ui_system& sys, button button)
{
    return (sys.input.mouse_button_state & (1 << int(button))) != 0;
}

bool
detect_mouse_press(ephemeral_context& ctx, button button)
{
    return (get_event_type(ctx) == ALIA_EVENT_MOUSE_PRESS
            || get_event_type(ctx) == ALIA_EVENT_DOUBLE_CLICK)
        && as_mouse_press_event(ctx).button == alia_button_t(button);
}

bool
detect_mouse_press(ephemeral_context& ctx, alia_element_id id, button button)
{
    if (detect_mouse_press(ctx, button) && is_element_hot(ctx, id))
    {
        set_element_with_capture(
            get_system(ctx), make_routable_element_id(ctx, id));
        return true;
    }
    else
    {
        return false;
    }
}

bool
detect_mouse_release(ephemeral_context& ctx, button button)
{
    return get_event_type(ctx) == ALIA_EVENT_MOUSE_RELEASE
        && as_mouse_release_event(ctx).button == alia_button_t(button);
}

bool
detect_mouse_release(ephemeral_context& ctx, alia_element_id id, button button)
{
    return detect_mouse_release(ctx, button) && element_has_capture(ctx, id);
}

bool
detect_mouse_motion(ephemeral_context& ctx, alia_element_id id)
{
    return (get_event_type(ctx) == ALIA_EVENT_MOUSE_MOTION
            && element_has_capture(ctx, id))
        || (no_element_has_capture(ctx) && is_element_hot(ctx, id));
}

bool
detect_double_click(ephemeral_context& ctx, button button)
{
    return get_event_type(ctx) == ALIA_EVENT_DOUBLE_CLICK
        && as_double_click_event(ctx).button == alia_button_t(button);
}

bool
detect_double_click(ephemeral_context& ctx, alia_element_id id, button button)
{
    return detect_double_click(ctx, button) && is_element_hot(ctx, id);
}

bool
detect_click(ephemeral_context& ctx, alia_element_id id, button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_mouse_release(ctx, id, button) && is_element_hot(ctx, id);
}

bool
is_click_possible(ui_system& sys, alia_element_id id)
{
    return is_element_hot(sys, id) && no_element_has_capture(sys);
}

bool
is_click_in_progress(ui_system& sys, alia_element_id id, button button)
{
    return is_element_hot(sys, id) && element_has_capture(sys, id)
        && is_mouse_button_pressed(sys, button);
}

bool
detect_drag(ephemeral_context& ctx, alia_element_id id, button button)
{
    detect_mouse_press(ctx, id, button);
    return get_event_type(ctx) == ALIA_EVENT_MOUSE_MOTION
        && is_mouse_button_pressed(ctx, button)
        && element_has_capture(ctx, id);
}

bool
detect_press_or_drag(ephemeral_context& ctx, alia_element_id id, button button)
{
    return (detect_mouse_press(ctx, id, button)
            || (get_event_type(ctx) == ALIA_EVENT_MOUSE_MOTION
                && is_mouse_button_pressed(ctx, button)))
        && element_has_capture(ctx, id);
}

vec2f
get_mouse_motion_delta(ephemeral_context& ctx, alia_element_id id)
{
    // TODO: Implement this.
    return vec2f{0, 0};
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
is_drag_in_progress(ui_system& sys, alia_element_id id, button button)
{
    return is_mouse_button_pressed(sys, button) && element_has_capture(sys, id)
        && sys.input.dragging;
}

bool
detect_drag_release(ephemeral_context& ctx, alia_element_id id, button button)
{
    return is_drag_in_progress(ctx, id, button)
        && detect_mouse_release(ctx, button);
}

bool
detect_stationary_click(
    ephemeral_context& ctx, alia_element_id id, button button)
{
    return detect_click(ctx, id, button) && !get_system(ctx).input.dragging;
}

bool
detect_mouse_gain(ephemeral_context& ctx, alia_element_id id)
{
    return get_event_type(ctx) == ALIA_EVENT_MOUSE_GAIN
        && get_event_target(ctx) == id;
}

bool
detect_mouse_loss(ephemeral_context& ctx, alia_element_id id)
{
    return get_event_type(ctx) == ALIA_EVENT_MOUSE_LOSS
        && get_event_target(ctx) == id;
}

bool
detect_scroll(ephemeral_context& ctx, alia_element_id id, vec2f* out_delta)
{
    if (get_event_type(ctx) == ALIA_EVENT_WHEEL && get_event_target(ctx) == id)
    {
        *out_delta = as_wheel_event(ctx).delta;
        return true;
    }
    else
    {
        return false;
    }
}

} // namespace alia
