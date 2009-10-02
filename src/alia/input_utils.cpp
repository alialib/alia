#include <alia/input_utils.hpp>
#include <alia/context.hpp>
#include <alia/input_events.hpp>
#include <alia/box.hpp>
#include <alia/surface.hpp>
#include <alia/region.hpp>
#include <alia/layout.hpp>

namespace alia {

void refresh_region_id(context& ctx, region_id id)
{
    id->container = get_event<refresh_event>(ctx).active_data;
    id->last_refresh = ctx.refresh_counter;
}

region_id get_region_id(context& ctx)
{
    region_id id = get_data<region_data>(ctx);
    if (ctx.event->type == REFRESH_EVENT)
        refresh_region_id(ctx, id);
    return id;
}

void hit_test_region(context& ctx, region_id id, box2i const& box)
{
    hit_test_event& e = get_event<hit_test_event>(ctx);
    if (is_inside(box, e.integer_untransformed_p) &&
        is_inside(ctx.pass_state.clip_region, e.p))
    {
        e.id = id;
    }
}

void do_region_visibility(context& ctx, region_id id, box2i const& region)
{
    make_region_visible_event& e = get_event<make_region_visible_event>(ctx);
    if (e.target_id == id)
        e.region = region;
}

void do_region(context& ctx, region_id id, box2i const& region)
{
    switch (ctx.event->type)
    {
     case HIT_TEST_EVENT:
        hit_test_region(ctx, id, region);
        break;
     case MAKE_REGION_VISIBLE:
        do_region_visibility(ctx, id, region);
        break;
    }
}

void make_region_visible(context& ctx, region_id id)
{
    make_region_visible_event e(id);
    issue_event(ctx, e);
}

bool mouse_is_inside_region(context& ctx, box2i const& region)
{
    return ctx.mouse_in_surface &&
        is_inside(region, ctx.pass_state.integer_mouse_position) &&
        is_inside(ctx.surface->get_clip_region(), ctx.mouse_position);
}
bool mouse_is_inside_region(context& ctx, box2d const& region)
{
    return ctx.mouse_in_surface &&
        is_inside(region, ctx.pass_state.mouse_position) &&
        is_inside(ctx.surface->get_clip_region(), ctx.mouse_position);
}

// KEYBOARD

bool detect_key_press(context& ctx, key_event_info* info, region_id id)
{
    if (ctx.event->type == BUTTON_DOWN_EVENT && is_region_hot(ctx, id))
    {
        set_focus(ctx, id);
        return false;
    }
    return id_has_focus(ctx, id) && detect_key_press(ctx, info);

}
bool detect_key_press(context& ctx, key_event_info* info)
{
    if (ctx.event->type == KEY_DOWN_EVENT)
    {
        key_event& e = get_event<key_event>(ctx);
        if (!e.processed)
        {
            *info = e.info;
            return true;
        }
    }
    return false;
}

bool detect_key_release(context& ctx, key_event_info* info, region_id id)
{
    if (ctx.event->type == BUTTON_DOWN_EVENT && is_region_hot(ctx, id))
    {
        set_focus(ctx, id);
        return false;
    }
    return id_has_focus(ctx, id) && detect_key_release(ctx, info);
}
bool detect_key_release(context& ctx, key_event_info* info)
{
    if (ctx.event->type == KEY_UP_EVENT)
    {
        key_event& e = get_event<key_event>(ctx);
        if (!e.processed)
        {
            *info = e.info;
            return true;
        }
    }
    return false;
}

int detect_char(context& ctx, region_id id)
{
    if (ctx.event->type == BUTTON_DOWN_EVENT && is_region_hot(ctx, id))
    {
        set_focus(ctx, id);
        return 0;
    }
    if (id_has_focus(ctx, id))
        return detect_char(ctx);
    return 0;
}
int detect_char(context& ctx)
{
    if (ctx.event->type == CHAR_EVENT)
    {
        char_event& e = get_event<char_event>(ctx);
        if (!e.processed)
            return e.character;
    }
    return 0;
}

void acknowledge_input_event(context& ctx)
{
    get_event<targeted_input_event>(ctx).processed = true;
}

bool detect_key_press(context& ctx, region_id id,
    key_code code, int modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info, id) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_press(context& ctx, key_code code, int modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_release(context& ctx, region_id id,
    key_code code, int modifiers)
{
    key_event_info info;
    if (detect_key_release(ctx, &info, id) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_release(context& ctx, key_code code, int modifiers)
{
    key_event_info info;
    if (detect_key_release(ctx, &info) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_proper_key_release(context& ctx, int& state, region_id id,
    key_code code, int modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info, id))
    {
        if (info.code == code && info.mods == modifiers)
        {
            if (state == 0)
                state = 1;
        }
        else if (state == 1)
            state = 2;
    }
    else if (detect_key_release(ctx, id, code, modifiers))
    {
        bool proper = state == 1;
        state = 0;
        return proper;
    }
    return false;
}
bool detect_proper_key_release(context& ctx, int& state, key_code code,
    int modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info))
    {
        if (info.code == code && info.mods == modifiers)
        {
            if (state == 0)
                state = 1;
        }
        else if (state == 1)
            state = 2;
    }
    else if (detect_key_release(ctx, code, modifiers))
    {
        bool proper = state == 1;
        state = 0;
        return proper;
    }
    return false;
}

// MOUSE

bool is_mouse_in_surface(context& ctx)
{
    return ctx.mouse_in_surface;
}

point2i const& get_integer_mouse_position(context& ctx)
{
    return ctx.pass_state.integer_mouse_position;
}

bool is_mouse_button_pressed(context& ctx, mouse_button button)
{
    return (ctx.mouse_button_state & (1 << int(button))) != 0;
}

bool is_region_active(context& ctx, region_id id)
{
    return ctx.active_id == id;
}

bool is_region_hot(context& ctx, region_id id)
{
    return ctx.hot_id == id;
}

bool detect_mouse_down(context& ctx, mouse_button button)
{
    return (ctx.event->type == BUTTON_DOWN_EVENT ||
        ctx.event->type == DOUBLE_CLICK_EVENT) &&
        get_event<mouse_button_event>(ctx).button == button;
}

bool detect_mouse_down(context& ctx, region_id id, mouse_button button)
{
    if (detect_mouse_down(ctx, button) && is_region_hot(ctx, id))
    {
        ctx.active_id = id;
        return true;
    }
    else
        return false;
}

bool detect_mouse_up(context& ctx, mouse_button button)
{
    return ctx.event->type == BUTTON_UP_EVENT &&
        get_event<mouse_button_event>(ctx).button == button;
}

bool detect_mouse_up(context& ctx, region_id id,
    mouse_button button)
{
    return detect_mouse_up(ctx, button) && is_region_hot(ctx, id);
}

bool detect_double_click(context& ctx, region_id id, mouse_button button)
{
    return ctx.event->type == DOUBLE_CLICK_EVENT &&
        get_event<mouse_button_event>(ctx).button == button &&
        is_region_hot(ctx, id);
}

bool detect_click(context& ctx, region_id id, mouse_button button)
{
    detect_mouse_down(ctx, id, button);
    return detect_mouse_up(ctx, id, button) && is_region_active(ctx, id);
}

bool detect_potential_click(context& ctx, region_id id)
{
    return is_region_hot(ctx, id) && is_region_active(ctx, 0);
}

bool detect_click_in_progress(context& ctx, region_id id, mouse_button button)
{
    return ctx.hot_id == id && ctx.active_id == id &&
        is_mouse_button_pressed(ctx, button);
}

bool detect_drag(context& ctx, region_id id, mouse_button button)
{
    detect_mouse_down(ctx, id, button);
    return ctx.event->type == MOUSE_MOTION_EVENT &&
        is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id);
}

bool detect_drag_in_progress(context& ctx, region_id id, mouse_button button)
{
    return is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id);
}

bool detect_drag_release(context& ctx, region_id id, mouse_button button)
{
    return detect_mouse_up(ctx, button) && is_region_active(ctx, id);
}

//bool detect_stationary_click(context& ctx, region_id id, mouse_button button)
//{
//}
//
//bool detect_explicit_drag(context& ctx, region_id id, mouse_button button)
//{
//}
//
//bool detect_explicit_drag_release(context& ctx, region_id id,
//    mouse_button button)
//{
//}

int detect_wheel_movement(context& ctx)
{
    if (ctx.event->type == SCROLL_WHEEL_EVENT)
    {
        scroll_wheel_event& e = get_event<scroll_wheel_event>(ctx);
        if (!e.processed)
            return e.amount;
    }
    return 0;
}

// FOCUS

void add_to_focus_order(context& ctx, region_id id)
{
    switch (ctx.event->type)
    {
     case FOCUS_PREDECESSOR_EVENT:
      {
        focus_predecessor_event& e = get_event<focus_predecessor_event>(ctx);
        if (ctx.focused_id == id && e.id)
            e.saw_focus = true;
        if (!e.saw_focus)
            e.id = id;
        break;
      }
     case FOCUS_SUCCESSOR_EVENT:
      {
        focus_successor_event& e = get_event<focus_successor_event>(ctx);
        if (e.just_saw_focus)
        {
            e.id = id;
            e.just_saw_focus = false;
        }
        if (ctx.focused_id == id)
            e.just_saw_focus = true;
        break;
      }
    }
}

bool id_has_focus(context& ctx, region_id id)
{
    return ctx.focused_id == id;
}

void set_focus(context& ctx, region_id id)
{
    if (ctx.focused_id != id)
    {
        if (ctx.focused_id)
        {
            focus_loss_event e(ctx.focused_id);
            issue_event(ctx, e);
        }
        ctx.focused_id = id;
        if (id)
        {
            make_region_visible(ctx, id);
            focus_gain_event e(id);
            issue_event(ctx, e);
        }
    }
}

region_id get_id_before_focus(context& ctx)
{
    focus_predecessor_event e;
    issue_event(ctx, e);
    return e.id;
}

region_id get_id_after_focus(context& ctx)
{
    focus_successor_event e;
    issue_event(ctx, e);
    return e.id;
}

void set_initial_focus(context& ctx)
{
    ctx.focused_id = 0;
    set_focus(ctx, get_id_after_focus(ctx));
}

bool detect_focus_gain(context& ctx, region_id id)
{
    return ctx.event->type == FOCUS_GAIN_EVENT &&
        get_event<focus_gain_event>(ctx).target_id == id;
}

bool detect_focus_loss(context& ctx, region_id id)
{
    return ctx.event->type == FOCUS_LOSS_EVENT &&
        get_event<focus_loss_event>(ctx).target_id == id;
}

}
