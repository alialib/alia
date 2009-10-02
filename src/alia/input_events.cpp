#include <alia/input_events.hpp>
#include <alia/context.hpp>
#include <alia/input_utils.hpp>
#include <alia/transformations.hpp>

namespace alia {

region_id do_hit_test(context& ctx, point2i const& position)
{
    hit_test_event e(position);
    e.id = 0;
    issue_event(ctx, e);
    return e.id;
}

void update_hot_id(context& ctx)
{
    if (ctx.mouse_in_surface)
        ctx.hot_id = do_hit_test(ctx, ctx.mouse_position);
    else
        ctx.hot_id = 0;
}

void clear_mouse_position(context& ctx)
{
    ctx.mouse_in_surface = false;
}
void set_mouse_position(context& ctx, point2i const& position)
{
    // TODO: Fix this mess.
    if (!ctx.mouse_in_surface || ctx.mouse_position != position)
    {
        point2i old_position = ctx.mouse_position;
        ctx.mouse_in_surface = true;
        ctx.mouse_position = position;
        update_hot_id(ctx);
        if (ctx.mouse_in_surface)
        {
            mouse_motion_event e(ctx.active_id ? ctx.active_id : ctx.hot_id,
                old_position, position);
            issue_event(ctx, e);
        }
    }
}

vector2d get_drag_delta(context& ctx, mouse_motion_event& e)
{
    return
        transform_point(ctx.pass_state.inverse_transformation,
            point2d(e.new_position))
      - transform_point(ctx.pass_state.inverse_transformation,
            point2d(e.old_position));
}

void process_mouse_press(context& ctx, mouse_button button)
{
    ctx.mouse_button_state |= 1 << int(button);
    region_id id = ctx.active_id ? ctx.active_id : ctx.hot_id;
    {
        mouse_button_event e(BUTTON_DOWN_EVENT, button, id);
        issue_event(ctx, e);
    }
    if (!id)
        set_focus(ctx, 0);
}
void process_mouse_release(context& ctx, mouse_button button)
{
    ctx.mouse_button_state &= ~(1 << int(button));
    mouse_button_event e(BUTTON_UP_EVENT, button,
        ctx.active_id ? ctx.active_id : ctx.hot_id);
    issue_event(ctx, e);
    // TODO: how should this work?
    if (ctx.mouse_button_state == 0)
        ctx.active_id = 0;
}
void process_double_click(context& ctx, mouse_button button)
{
    ctx.mouse_button_state |= 1 << int(button);
    mouse_button_event e(DOUBLE_CLICK_EVENT, button,
        ctx.active_id ? ctx.active_id : ctx.hot_id);
    issue_event(ctx, e);
}

void process_scroll_wheel_movement(context& ctx, int amount)
{
    scroll_wheel_event e(amount, ctx.active_id ? ctx.active_id : ctx.hot_id);
    issue_event(ctx, e);
}

}
