#include <alia/indie/utilities/mouse.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/internals.hpp>
#include <alia/indie/system/api.hpp>

namespace alia { namespace indie {

bool
is_component_hot(system& sys, component_id id)
{
    return sys.input.hot_id.id == id;
}

bool
component_has_capture(system& sys, component_id id)
{
    return sys.input.id_with_capture.id == id;
}

bool
no_component_has_capture(system& sys)
{
    return !is_valid(sys.input.id_with_capture);
}

vector<2, double>
get_mouse_position(dataless_context ctx)
{
    return transform(
        inverse(get_layout_traversal(ctx).geometry->transformation_matrix),
        vector<2, double>(get_system(ctx).input.mouse_position));
}

vector<2, int>
get_integer_mouse_position(dataless_context ctx)
{
    vector<2, double> dp = get_mouse_position(ctx);
    return make_vector<int>(int(dp[0] + 0.5), int(dp[1] + 0.5));
}

bool
is_mouse_in_surface(system& sys)
{
    return sys.input.mouse_inside_window;
}

bool
is_mouse_button_pressed(system& sys, mouse_button button)
{
    return (sys.input.mouse_button_state & (1 << int(button))) != 0;
}

bool
detect_mouse_press(dataless_context ctx, mouse_button button)
{
    mouse_button_event* event;
    return detect_event(ctx, &event) && event->button == button
           && (event->type == input_event_type::MOUSE_PRESS
               || event->type == input_event_type::DOUBLE_CLICK);
}

bool
detect_mouse_press(dataless_context ctx, component_id id, mouse_button button)
{
    if (detect_mouse_press(ctx, button) && is_component_hot(ctx, id))
    {
        set_component_with_capture(get_system(ctx), externalize(id));
        return true;
    }
    else
    {
        return false;
    }
}

bool
detect_mouse_release(dataless_context ctx, mouse_button button)
{
    mouse_button_event* event;
    return detect_event(ctx, &event) && event->button == button
           && event->type == input_event_type::MOUSE_RELEASE;
}

bool
detect_mouse_release(
    dataless_context ctx, component_id id, mouse_button button)
{
    return detect_mouse_release(ctx, button) && component_has_capture(ctx, id);
}

bool
detect_mouse_motion(dataless_context ctx, component_id id)
{
    mouse_motion_event* event;
    return detect_event(ctx, &event) && component_has_capture(ctx, id)
           || no_component_has_capture(ctx) && is_component_hot(ctx, id);
}

bool
detect_double_click(dataless_context ctx, mouse_button button)
{
    mouse_button_event* event;
    return detect_event(ctx, &event) && event->button == button
           && event->type == input_event_type::DOUBLE_CLICK;
}

bool
detect_double_click(dataless_context ctx, component_id id, mouse_button button)
{
    return detect_double_click(ctx, button) && is_component_hot(ctx, id);
}

bool
detect_click(dataless_context ctx, component_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_mouse_release(ctx, id, button) && is_component_hot(ctx, id);
}

bool
is_click_possible(system& sys, component_id id)
{
    return is_component_hot(sys, id) && no_component_has_capture(sys);
}

bool
is_click_in_progress(system& sys, component_id id, mouse_button button)
{
    return is_component_hot(sys, id) && component_has_capture(sys, id)
           && is_mouse_button_pressed(sys, button);
}

#if 0

bool
detect_drag(dataless_context ctx, component_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_event(ctx, MOUSE_MOTION_EVENT)
           && is_mouse_button_pressed(ctx, button)
           && component_has_capture(ctx, id);
}

bool
detect_press_or_drag(
    dataless_context ctx, component_id id, mouse_button button)
{
    return (detect_mouse_press(ctx, id, button)
            || detect_event(ctx, MOUSE_MOTION_EVENT)
                   && is_mouse_button_pressed(ctx, button))
           && component_has_capture(ctx, id);
}

vector<2, double>
get_drag_delta(dataless_context ctx)
{
    mouse_motion_event& e = get_event<mouse_motion_event>(ctx);
    matrix<3, 3, double> m = inverse(get_transformation(ctx));
    return transform(
               m, vector<2, double>(get_system(ctx).input.mouse_position))
           - transform(m, vector<2, double>(e.last_mouse_position));
}

bool
is_drag_in_progress(dataless_context ctx, component_id id, mouse_button button)
{
    return is_mouse_button_pressed(ctx, button)
           && component_has_capture(ctx, id) && get_system(ctx).input.dragging;
}

bool
detect_drag_release(dataless_context ctx, component_id id, mouse_button button)
{
    return is_drag_in_progress(ctx, id, button)
           && detect_mouse_release(ctx, button);
}

bool
detect_stationary_click(
    dataless_context ctx, component_id id, mouse_button button)
{
    return detect_click(ctx, id, button) && !get_system(ctx).input.dragging;
}

bool
detect_wheel_movement(dataless_context ctx, float* movement, component_id id)
{
    if (ctx.event->type == MOUSE_WHEEL_EVENT)
    {
        mouse_wheel_event& e = get_event<mouse_wheel_event>(ctx);
        if (e.target == id)
        {
            *movement = e.movement;
            acknowledge_input_event(ctx);
            return true;
        }
    }
    return false;
}

bool
detect_mouse_gain(dataless_context ctx, component_id id)
{
    if (ctx.event->type == MOUSE_GAIN_EVENT)
    {
        mouse_notification_event& event
            = get_event<mouse_notification_event>(ctx);
        return event.target == id;
    }
    return false;
}

bool
detect_mouse_loss(dataless_context ctx, component_id id)
{
    if (ctx.event->type == MOUSE_LOSS_EVENT)
    {
        mouse_notification_event& event
            = get_event<mouse_notification_event>(ctx);
        return event.target == id;
    }
    return false;
}

#endif

}} // namespace alia::indie
