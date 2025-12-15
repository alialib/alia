#include <alia/system/input_processing.hpp>

#include <alia/events.hpp>
#include <alia/flow/dispatch.hpp>
#include <alia/system/interface.hpp>
#include <alia/system/object.hpp>

namespace alia {

namespace {

routable_widget_id
get_mouse_target(ui_system& ui)
{
    // If no widget has capture, send events to the widget under the mouse.
    return ui.input.widget_with_capture ? ui.input.widget_with_capture
                                        : ui.input.hot_widget;
}

} // namespace

void
process_mouse_motion(ui_system& ui, vec2 const& position)
{
    if (!ui.input.mouse_inside_window || ui.input.mouse_position != position)
    {
        alia_event event
            = alia_make_mouse_motion_event({.x = position.x, .y = position.y});
        dispatch_targeted_event(ui, event, get_mouse_target(ui));
        refresh_system(ui);

        ui.input.mouse_position = position;
        ui.input.mouse_inside_window = true;

        if (ui.input.mouse_button_state != 0)
            ui.input.dragging = true;
    }
}

void
process_mouse_loss(ui_system& ui)
{
    ui.input.mouse_inside_window = false;
}

void
process_mouse_press(
    ui_system& ui,
    vec2 const& position,
    mouse_button button,
    key_modifiers mods)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        alia_event event = alia_make_mouse_press_event(
            {.button = int(button),
             .mods = raw_code(mods),
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(ui, event, target);
        refresh_system(ui);
    }
    else
    {
        // TODO
        // clear_focus(ui);
    }
    ui.input.last_mouse_press_time[unsigned(button)] = ui.tick_count;
    ui.input.mouse_button_state |= 1 << mouse_button_code(button);
    ui.input.keyboard_interaction = false;
}

void
process_mouse_release(
    ui_system& ui,
    vec2 const& position,
    mouse_button button,
    key_modifiers mods)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        alia_event event = alia_make_mouse_release_event(
            {.button = int(button),
             .mods = raw_code(mods),
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(ui, event, target);
        refresh_system(ui);
    }
    ui.input.mouse_button_state &= ~(1 << int(button));
    if (ui.input.mouse_button_state == 0)
    {
        // TODO
        // set_widget_with_capture(ui, routable_widget_id{});
        ui.input.dragging = false;
    }
}

void
process_double_click(
    ui_system& ui,
    vec2 const& position,
    mouse_button button,
    key_modifiers mods)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        alia_event event = alia_make_double_click_event(
            {.button = int(button),
             .mods = raw_code(mods),
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(ui, event, target);
    }
    ui.input.mouse_button_state |= 1 << int(button);
    ui.input.keyboard_interaction = false;
}

#if 0

void
process_scroll(ui_system& ui, vec2 const& delta)
{
    alia_event hit_test = alia_make_wheel_hit_test_event(
        {.x = ui.input.mouse_position.x,
         .y = ui.input.mouse_position.y,
         .delta = delta});
    dispatch_event(ui, hit_test, WHEEL_HIT_TEST_EVENT);
    if (hit_test.result)
    {
        scroll_event event{{}, delta};
        dispatch_targeted_event(ui, event, *hit_test.result, SCROLL_EVENT);
        refresh_system(ui);
    }
}

bool
process_focused_key_press(ui_system& ui, modded_key const& info)
{
    key_event event{{}, info};
    dispatch_targeted_event(
        ui, event, ui.input.widget_with_focus, KEY_PRESS_EVENT);
    if (event.acknowledged)
        refresh_system(ui);
    return event.acknowledged;
}

bool
process_background_key_press(ui_system& ui, modded_key const& info)
{
    key_event event{{}, info};
    dispatch_event(ui, event, BACKGROUND_KEY_PRESS_EVENT);
    if (event.acknowledged)
        refresh_system(ui);
    return event.acknowledged;
}

bool
process_focused_key_release(ui_system& ui, modded_key const& info)
{
    key_event event{{}, info};
    dispatch_targeted_event(
        ui, event, ui.input.widget_with_focus, KEY_RELEASE_EVENT);
    if (event.acknowledged)
        refresh_system(ui);
    return event.acknowledged;
}
bool
process_key_press(ui_system& ui, modded_key const& info)
{
    ui.input.keyboard_interaction = true;
    return process_focused_key_press(ui, info)
        || process_background_key_press(ui, info);
}

bool
process_key_release(ui_system& ui, modded_key const& info)
{
    ui.input.keyboard_interaction = true;
    return process_focused_key_release(ui, info);
}

#endif

} // namespace alia
