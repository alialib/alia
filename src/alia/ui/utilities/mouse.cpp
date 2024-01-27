#include <alia/indie/utilities/mouse.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/node_interface.hpp>
#include <alia/indie/system/api.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

bool
is_element_hot(system& sys, internal_element_ref element)
{
    return sys.input.hot_element.matches(element);
}

bool
element_has_capture(system& sys, internal_element_ref element)
{
    return sys.input.element_with_capture.matches(element);
}

bool
no_element_has_capture(system& sys)
{
    return !sys.input.element_with_capture;
}

vector<2, double>
get_mouse_position(system& sys, widget const& widget)
{
    return transform(
        inverse(widget.transformation()),
        vector<2, double>(sys.input.mouse_position));
}

vector<2, int>
get_integer_mouse_position(system& sys, widget const& widget)
{
    vector<2, double> dp = get_mouse_position(sys, widget);
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
detect_mouse_press(event_context ctx, mouse_button button)
{
    mouse_button_event* event;
    return detect_event(ctx, &event) && event->button == button
           && (event->type == input_event_type::MOUSE_PRESS
               || event->type == input_event_type::DOUBLE_CLICK);
}

bool
detect_mouse_press(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    if (detect_mouse_press(ctx, button) && is_element_hot(ctx, element))
    {
        set_element_with_capture(get_system(ctx), externalize(element));
        return true;
    }
    else
    {
        return false;
    }
}

bool
detect_mouse_release(event_context ctx, mouse_button button)
{
    mouse_button_event* event;
    return detect_event(ctx, &event) && event->button == button
           && event->type == input_event_type::MOUSE_RELEASE;
}

bool
detect_mouse_release(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    return detect_mouse_release(ctx, button)
           && element_has_capture(ctx, element);
}

bool
detect_mouse_motion(event_context ctx, internal_element_ref element)
{
    mouse_motion_event* event;
    return detect_event(ctx, &event) && element_has_capture(ctx, element)
           || no_element_has_capture(ctx) && is_element_hot(ctx, element);
}

bool
detect_double_click(event_context ctx, mouse_button button)
{
    mouse_button_event* event;
    return detect_event(ctx, &event) && event->button == button
           && event->type == input_event_type::DOUBLE_CLICK;
}

bool
detect_double_click(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    return detect_double_click(ctx, button) && is_element_hot(ctx, element);
}

bool
detect_click(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    detect_mouse_press(ctx, element, button);
    return detect_mouse_release(ctx, element, button)
           && is_element_hot(ctx, element);
}

bool
is_click_possible(system& sys, internal_element_ref element)
{
    return is_element_hot(sys, element) && no_element_has_capture(sys);
}

bool
is_click_in_progress(
    system& sys, internal_element_ref element, mouse_button button)
{
    return is_element_hot(sys, element) && element_has_capture(sys, element)
           && is_mouse_button_pressed(sys, button);
}

bool
detect_drag(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    detect_mouse_press(ctx, element, button);
    mouse_motion_event* event;
    return detect_event(ctx, &event) && is_mouse_button_pressed(ctx, button)
           && element_has_capture(ctx, element);
}

bool
detect_press_or_drag(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    mouse_motion_event* event;
    return (detect_mouse_press(ctx, element, button)
            || (detect_event(ctx, &event)
                && is_mouse_button_pressed(ctx, button)))
           && element_has_capture(ctx, element);
}

vector<2, double>
get_mouse_motion_delta(event_context ctx, widget const& widget)
{
    mouse_motion_event* event;
    if (!detect_event(ctx, &event))
        return make_vector<double>(0, 0);
    matrix<3, 3, double> matrix = inverse(widget.transformation());
    // The system's `mouse_position` field is updated immediately after the
    // mouse_motion_event is delivered, so by subtracting that from the one
    // carried by the event, we get the delta.
    return transform(matrix, vector<2, double>(event->position))
           - transform(
               matrix,
               vector<2, double>(get_system(ctx).input.mouse_position));
}

bool
is_drag_in_progress(
    system& sys, internal_element_ref element, mouse_button button)
{
    return is_mouse_button_pressed(sys, button)
           && element_has_capture(sys, element) && sys.input.dragging;
}

bool
detect_drag_release(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    return is_drag_in_progress(ctx, element, button)
           && detect_mouse_release(ctx, button);
}

bool
detect_stationary_click(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    return detect_click(ctx, element, button)
           && !get_system(ctx).input.dragging;
}

#if 0

bool
detect_wheel_movement(
    event_context ctx, float* movement, internal_element_ref element)
{
    if (ctx.event->type == MOUSE_WHEEL_EVENT)
    {
        mouse_wheel_event& e = get_event<mouse_wheel_event>(ctx);
        if (e.target == element)
        {
            *movement = e.movement;
            acknowledge_input_event(ctx);
            return true;
        }
    }
    return false;
}

bool
detect_mouse_gain(event_context ctx, internal_element_ref element)
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
detect_mouse_loss(event_context ctx, internal_element_ref element)
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
