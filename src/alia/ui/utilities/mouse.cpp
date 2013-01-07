#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

vector<2,double> get_mouse_position(ui_context& ctx)
{
    return transform(inverse(get_transformation(ctx)),
        vector<2,double>(ctx.system->input.mouse_position));
}
vector<2,int> get_integer_mouse_position(ui_context& ctx)
{
    vector<2,double> dp = get_mouse_position(ctx);
    return make_vector<int>(int(dp[0] + 0.5), int(dp[1] + 0.5));
}

bool mouse_is_inside_box(ui_context& ctx, box<2,double> const& box)
{
    return
        ctx.system->input.mouse_inside_window &&
        is_inside(box, get_mouse_position(ctx)) &&
        is_inside(get_geometry_context(ctx).clip_region,
            vector<2,double>(ctx.system->input.mouse_position));
}

bool is_mouse_button_pressed(ui_context& ctx, mouse_button button)
{
    return (ctx.system->input.mouse_button_state & (1 << int(button))) != 0;
}

bool detect_mouse_press(ui_context& ctx, mouse_button button)
{
    return (detect_event(ctx, MOUSE_PRESS_EVENT) ||
        detect_event(ctx, DOUBLE_CLICK_EVENT)) &&
        get_event<mouse_button_event>(ctx).button == button;
}

bool detect_mouse_press(ui_context& ctx, widget_id id, mouse_button button)
{
    if (detect_mouse_press(ctx, button) && is_region_hot(ctx, id))
    {
        ctx.system->input.active_id = make_routable_widget_id(ctx, id);
        return true;
    }
    else
        return false;
}

bool detect_mouse_release(ui_context& ctx, mouse_button button)
{
    return detect_event(ctx, MOUSE_RELEASE_EVENT) &&
        get_event<mouse_button_event>(ctx).button == button;
}
bool detect_mouse_release(ui_context& ctx, widget_id id,
    mouse_button button)
{
    return detect_mouse_release(ctx, button) && is_region_active(ctx, id);
}

bool detect_mouse_motion(ui_context& ctx, widget_id id)
{
    return detect_event(ctx, MOUSE_MOTION_EVENT) && is_region_hot(ctx, id);
}

bool detect_double_click(ui_context& ctx, widget_id id, mouse_button button)
{
    return detect_event(ctx, DOUBLE_CLICK_EVENT) &&
        get_event<mouse_button_event>(ctx).button == button &&
        is_region_hot(ctx, id);
}
bool detect_click(ui_context& ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_mouse_release(ctx, id, button) && is_region_active(ctx, id)
        && is_region_hot(ctx, id);
}

bool detect_potential_click(ui_context& ctx, widget_id id)
{
    return is_region_hot(ctx, id) && is_region_active(ctx, 0);
}

bool detect_click_in_progress(ui_context& ctx, widget_id id,
    mouse_button button)
{
    return is_region_hot(ctx, id) && is_region_active(ctx, id) &&
        is_mouse_button_pressed(ctx, button);
}

bool detect_drag(ui_context& ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_event(ctx, MOUSE_MOTION_EVENT) &&
        is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id);
}

bool detect_mouse_down(ui_context& ctx, widget_id id, mouse_button button)
{
    return (detect_mouse_press(ctx, id, button) ||
        detect_event(ctx, MOUSE_MOTION_EVENT) &&
        is_mouse_button_pressed(ctx, button)) && is_region_active(ctx, id);
}

vector<2,double> get_drag_delta(ui_context& ctx)
{
    mouse_motion_event& e = get_event<mouse_motion_event>(ctx);
    matrix<3,3,double> m = inverse(get_transformation(ctx));
    return transform(m, vector<2,double>(ctx.system->input.mouse_position)) -
        transform(m, vector<2,double>(e.last_mouse_position));
}

bool detect_drag_in_progress(ui_context& ctx, widget_id id,
    mouse_button button)
{
    return is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id);
}

bool detect_drag_release(ui_context& ctx, widget_id id, mouse_button button)
{
    return detect_mouse_release(ctx, button) && is_region_active(ctx, id);
}

bool detect_wheel_movement(ui_context& ctx, float* movement, widget_id id)
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

}
