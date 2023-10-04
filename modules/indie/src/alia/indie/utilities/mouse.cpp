#include <alia/indie/utilities/mouse.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/node_interface.hpp>
#include <alia/indie/system/api.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

bool
is_widget_hot(system& sys, widget const* widget)
{
    return sys.input.hot_widget.matches(widget);
}

bool
widget_has_capture(system& sys, widget const* widget)
{
    return sys.input.widget_with_capture.matches(widget);
}

bool
no_component_has_capture(system& sys)
{
    return !sys.input.widget_with_capture;
}

// vector<2, double>
// get_mouse_position(dataless_context ctx)
// {
//     return transform(
//         inverse(get_layout_traversal(ctx).geometry->transformation_matrix),
//         vector<2, double>(get_system(ctx).input.mouse_position));
// }

// vector<2, int>
// get_integer_mouse_position(dataless_context ctx)
// {
//     vector<2, double> dp = get_mouse_position(ctx);
//     return make_vector<int>(int(dp[0] + 0.5), int(dp[1] + 0.5));
// }

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
    event_context ctx, widget const* widget, mouse_button button)
{
    if (detect_mouse_press(ctx, button) && is_widget_hot(ctx, widget))
    {
        set_widget_with_capture(get_system(ctx), externalize(widget));
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
    event_context ctx, widget const* widget, mouse_button button)
{
    return detect_mouse_release(ctx, button)
           && widget_has_capture(ctx, widget);
}

bool
detect_mouse_motion(event_context ctx, widget const* widget)
{
    mouse_motion_event* event;
    return detect_event(ctx, &event) && widget_has_capture(ctx, widget)
           || no_component_has_capture(ctx) && is_widget_hot(ctx, widget);
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
    event_context ctx, widget const* widget, mouse_button button)
{
    return detect_double_click(ctx, button) && is_widget_hot(ctx, widget);
}

bool
detect_click(event_context ctx, widget const* widget, mouse_button button)
{
    detect_mouse_press(ctx, widget, button);
    return detect_mouse_release(ctx, widget, button)
           && is_widget_hot(ctx, widget);
}

bool
is_click_possible(system& sys, widget const* widget)
{
    return is_widget_hot(sys, widget) && no_component_has_capture(sys);
}

bool
is_click_in_progress(system& sys, widget const* widget, mouse_button button)
{
    return is_widget_hot(sys, widget) && widget_has_capture(sys, widget)
           && is_mouse_button_pressed(sys, button);
}

#if 0

bool
detect_drag(event_context ctx, widget const* widget, mouse_button button)
{
    detect_mouse_press(ctx, widget, button);
    return detect_event(ctx, MOUSE_MOTION_EVENT)
           && is_mouse_button_pressed(ctx, button)
           && component_has_capture(ctx, widget);
}

bool
detect_press_or_drag(
    event_context ctx, widget const* widget, mouse_button button)
{
    return (detect_mouse_press(ctx, widget, button)
            || detect_event(ctx, MOUSE_MOTION_EVENT)
                   && is_mouse_button_pressed(ctx, button))
           && component_has_capture(ctx, widget);
}

vector<2, double>
get_drag_delta(event_context ctx)
{
    mouse_motion_event& e = get_event<mouse_motion_event>(ctx);
    matrix<3, 3, double> m = inverse(get_transformation(ctx));
    return transform(
               m, vector<2, double>(get_system(ctx).input.mouse_position))
           - transform(m, vector<2, double>(e.last_mouse_position));
}

bool
is_drag_in_progress(
    event_context ctx, widget const* widget, mouse_button button)
{
    return is_mouse_button_pressed(ctx, button)
           && component_has_capture(ctx, widget)
           && get_system(ctx).input.dragging;
}

bool
detect_drag_release(
    event_context ctx, widget const* widget, mouse_button button)
{
    return is_drag_in_progress(ctx, widget, button)
           && detect_mouse_release(ctx, button);
}

bool
detect_stationary_click(
    event_context ctx, widget const* widget, mouse_button button)
{
    return detect_click(ctx, widget, button)
           && !get_system(ctx).input.dragging;
}

bool
detect_wheel_movement(
    event_context ctx, float* movement, widget const* widget)
{
    if (ctx.event->type == MOUSE_WHEEL_EVENT)
    {
        mouse_wheel_event& e = get_event<mouse_wheel_event>(ctx);
        if (e.target == widget)
        {
            *movement = e.movement;
            acknowledge_input_event(ctx);
            return true;
        }
    }
    return false;
}

bool
detect_mouse_gain(event_context ctx, widget const* widget)
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
detect_mouse_loss(event_context ctx, widget const* widget)
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
