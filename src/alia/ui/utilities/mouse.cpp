#include <alia/ui/utilities/mouse.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/object.hpp>

namespace alia {

vector<2, double>
get_mouse_position(dataless_ui_context ctx)
{
    return transform(
        inverse(get_transformation(ctx)),
        vector<2, double>(get_system(ctx).input.mouse_position));
}
vector<2, int>
get_integer_mouse_position(dataless_ui_context ctx)
{
    vector<2, double> dp = get_mouse_position(ctx);
    return make_vector<int>(int(dp[0] + 0.5), int(dp[1] + 0.5));
}

bool
is_widget_hot(ui_system& sys, widget_id id)
{
    return sys.input.hot_widget.matches(id);
}

bool
widget_has_capture(ui_system& sys, widget_id id)
{
    return sys.input.widget_with_capture.matches(id);
}

bool
no_widget_has_capture(ui_system& sys)
{
    return !sys.input.widget_with_capture;
}

bool
is_mouse_in_surface(ui_system& sys)
{
    return sys.input.mouse_inside_window;
}

bool
is_mouse_button_pressed(ui_system& sys, mouse_button button)
{
    return (sys.input.mouse_button_state & (1 << int(button))) != 0;
}

bool
detect_mouse_press(dataless_ui_context ctx, mouse_button button)
{
    return (get_event_type(ctx) == MOUSE_PRESS_EVENT
            || get_event_type(ctx) == DOUBLE_CLICK_EVENT)
           && cast_event<mouse_button_event>(ctx).button == button;
}

bool
detect_mouse_press(dataless_ui_context ctx, widget_id id, mouse_button button)
{
    if (detect_mouse_press(ctx, button) && is_widget_hot(ctx, id))
    {
        set_widget_with_capture(
            get_system(ctx), make_routable_widget_id(ctx, id));
        return true;
    }
    else
    {
        return false;
    }
}

bool
detect_mouse_release(dataless_ui_context ctx, mouse_button button)
{
    return get_event_type(ctx) == MOUSE_RELEASE_EVENT
           && cast_event<mouse_button_event>(ctx).button == button;
}

bool
detect_mouse_release(
    dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return detect_mouse_release(ctx, button) && widget_has_capture(ctx, id);
}

bool
detect_mouse_motion(dataless_ui_context ctx, widget_id id)
{
    mouse_motion_event* event;
    return (detect_event(ctx, &event) && widget_has_capture(ctx, id))
           || (no_widget_has_capture(ctx) && is_widget_hot(ctx, id));
}

bool
detect_double_click(dataless_ui_context ctx, mouse_button button)
{
    return get_event_type(ctx) == DOUBLE_CLICK_EVENT
           && cast_event<mouse_button_event>(ctx).button == button;
}

bool
detect_double_click(dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return detect_double_click(ctx, button) && is_widget_hot(ctx, id);
}

bool
detect_click(dataless_ui_context ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_mouse_release(ctx, id, button) && is_widget_hot(ctx, id);
}

bool
is_click_possible(ui_system& sys, widget_id id)
{
    return is_widget_hot(sys, id) && no_widget_has_capture(sys);
}

bool
is_click_in_progress(ui_system& sys, widget_id id, mouse_button button)
{
    return is_widget_hot(sys, id) && widget_has_capture(sys, id)
           && is_mouse_button_pressed(sys, button);
}

bool
detect_drag(dataless_ui_context ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    mouse_motion_event* event;
    return detect_event(ctx, &event) && is_mouse_button_pressed(ctx, button)
           && widget_has_capture(ctx, id);
}

bool
detect_press_or_drag(
    dataless_ui_context ctx, widget_id id, mouse_button button)
{
    mouse_motion_event* event;
    return (detect_mouse_press(ctx, id, button)
            || (detect_event(ctx, &event)
                && is_mouse_button_pressed(ctx, button)))
           && widget_has_capture(ctx, id);
}

vector<2, double>
get_mouse_motion_delta(dataless_ui_context ctx, widget const& /*widget*/)
{
    mouse_motion_event* event;
    if (!detect_event(ctx, &event))
        return make_vector<double>(0, 0);
    matrix<3, 3, double> matrix = inverse(get_transformation(ctx));
    // The system's `mouse_position` field is updated immediately after the
    // mouse_motion_event is delivered, so by subtracting that from the one
    // carried by the event, we get the delta.
    return transform(matrix, vector<2, double>(event->position))
           - transform(
               matrix,
               vector<2, double>(get_system(ctx).input.mouse_position));
}

bool
is_drag_in_progress(ui_system& sys, widget_id id, mouse_button button)
{
    return is_mouse_button_pressed(sys, button) && widget_has_capture(sys, id)
           && sys.input.dragging;
}

bool
detect_drag_release(dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return is_drag_in_progress(ctx, id, button)
           && detect_mouse_release(ctx, button);
}

bool
detect_stationary_click(
    dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return detect_click(ctx, id, button) && !get_system(ctx).input.dragging;
}

bool
detect_mouse_gain(dataless_ui_context ctx, widget_id id)
{
    return get_event_type(ctx) == MOUSE_GAIN_EVENT
           && cast_event<mouse_notification_event>(ctx).target == id;
}

bool
detect_mouse_loss(dataless_ui_context ctx, widget_id id)
{
    return get_event_type(ctx) == MOUSE_LOSS_EVENT
           && cast_event<mouse_notification_event>(ctx).target == id;
}

std::optional<vector<2, double>>
detect_scroll(dataless_ui_context ctx, widget_id)
{
    scroll_event* event;
    if (detect_event(ctx, &event))
        return event->delta;
    else
        return std::nullopt;
}

} // namespace alia
