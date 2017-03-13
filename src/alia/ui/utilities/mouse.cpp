#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/system.hpp>

namespace alia {

vector<2,double> get_mouse_position(dataless_ui_context& ctx)
{
    return transform(inverse(get_transformation(ctx)),
        vector<2,double>(ctx.system->input.mouse_position));
}
vector<2,int> get_integer_mouse_position(dataless_ui_context& ctx)
{
    vector<2,double> dp = get_mouse_position(ctx);
    return make_vector<int>(int(dp[0] + 0.5), int(dp[1] + 0.5));
}

bool is_mouse_in_surface(dataless_ui_context& ctx)
{
    return ctx.system->input.mouse_inside_window;
}

bool is_mouse_button_pressed(dataless_ui_context& ctx, mouse_button button)
{
    return (ctx.system->input.mouse_button_state & (1 << int(button))) != 0;
}

bool detect_mouse_press(dataless_ui_context& ctx, mouse_button button)
{
    return (detect_event(ctx, MOUSE_PRESS_EVENT) ||
        detect_event(ctx, DOUBLE_CLICK_EVENT)) &&
        get_event<mouse_button_event>(ctx).button == button;
}

bool detect_mouse_press(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    if (detect_mouse_press(ctx, button) && is_region_hot(ctx, id))
    {
        set_active_region(*ctx.system, make_routable_widget_id(ctx, id));
        return true;
    }
    else
        return false;
}

bool detect_mouse_release(dataless_ui_context& ctx, mouse_button button)
{
    return detect_event(ctx, MOUSE_RELEASE_EVENT) &&
        get_event<mouse_button_event>(ctx).button == button;
}
bool detect_mouse_release(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    return detect_mouse_release(ctx, button) && is_region_active(ctx, id);
}

bool detect_mouse_motion(dataless_ui_context& ctx, widget_id id)
{
    return detect_event(ctx, MOUSE_MOTION_EVENT) && is_region_hot(ctx, id);
}

bool detect_double_click(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    return detect_event(ctx, DOUBLE_CLICK_EVENT) &&
        get_event<mouse_button_event>(ctx).button == button &&
        is_region_hot(ctx, id);
}
bool detect_click(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_mouse_release(ctx, id, button) && is_region_hot(ctx, id);
}

bool is_click_possible(dataless_ui_context& ctx, widget_id id)
{
    return is_region_hot(ctx, id) && is_region_active(ctx, 0);
}

bool is_click_in_progress(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    return is_region_hot(ctx, id) && is_region_active(ctx, id) &&
        is_mouse_button_pressed(ctx, button);
}

bool detect_drag(dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_event(ctx, MOUSE_MOTION_EVENT) &&
        is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id);
}

bool detect_press_or_drag(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    return (detect_mouse_press(ctx, id, button) ||
        detect_event(ctx, MOUSE_MOTION_EVENT) &&
        is_mouse_button_pressed(ctx, button)) && is_region_active(ctx, id);
}

vector<2,double> get_drag_delta(dataless_ui_context& ctx)
{
    mouse_motion_event& e = get_event<mouse_motion_event>(ctx);
    matrix<3,3,double> m = inverse(get_transformation(ctx));
    return transform(m, vector<2,double>(ctx.system->input.mouse_position)) -
        transform(m, vector<2,double>(e.last_mouse_position));
}

bool is_drag_in_progress(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    return is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id) &&
        ctx.system->input.dragging;
}

bool detect_drag_release(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    return is_drag_in_progress(ctx, id, button) &&
        detect_mouse_release(ctx, button);
}

bool detect_stationary_click(
    dataless_ui_context& ctx, widget_id id, mouse_button button)
{
    return detect_click(ctx, id, button) && !ctx.system->input.dragging;
}

bool detect_wheel_movement(
    dataless_ui_context& ctx, float* movement, widget_id id)
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

bool detect_mouse_gain(dataless_ui_context& ctx, widget_id id)
{
    if (ctx.event->type == MOUSE_GAIN_EVENT)
    {
        mouse_notification_event& event =
            get_event<mouse_notification_event>(ctx);
        return event.target == id;
    }
    return false;
}

bool detect_mouse_loss(dataless_ui_context& ctx, widget_id id)
{
    if (ctx.event->type == MOUSE_LOSS_EVENT)
    {
        mouse_notification_event& event =
            get_event<mouse_notification_event>(ctx);
        return event.target == id;
    }
    return false;
}

}
