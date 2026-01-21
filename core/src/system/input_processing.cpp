#include <alia/system/input_processing.hpp>

#include <alia/events.hpp>
#include <alia/flow/dispatch.hpp>
#include <alia/geometry.hpp>
#include <alia/system/interface.hpp>
#include <alia/system/object.hpp>

// TODO: API shouldn't be needed here.
#include <alia/system/api.hpp>

using namespace alia::operators;

namespace alia {

namespace {

alia_routable_element_id
get_mouse_target(ui_system& ui)
{
    // If no widget has capture, send events to the widget under the mouse.
    return alia_routable_element_id_is_valid(ui.input.element_with_capture)
             ? ui.input.element_with_capture
             : ui.input.hot_element;
}

} // namespace

void
process_mouse_motion(ui_system& ui, alia_vec2f const& position)
{
    if (!ui.input.mouse_inside_window
        || !alia_vec2f_equal(ui.input.mouse_position, position))
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
    ui_system& ui, vec2f const& position, button button, key_modifiers mods)
{
    auto target = get_mouse_target(ui);
    if (alia_routable_element_id_is_valid(target))
    {
        alia_event event = alia_make_mouse_press_event(
            {.button = alia_button_t(button),
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
    ui.input.mouse_button_state |= 1 << unsigned(button);
    ui.input.keyboard_interaction = false;
}

void
process_mouse_release(
    ui_system& ui, vec2f const& position, button button, key_modifiers mods)
{
    auto target = get_mouse_target(ui);
    if (alia_routable_element_id_is_valid(target))
    {
        alia_event event = alia_make_mouse_release_event(
            {.button = alia_button_t(button),
             .mods = raw_code(mods),
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(ui, event, target);
        refresh_system(ui);
    }
    ui.input.mouse_button_state &= ~(1 << unsigned(button));
    if (ui.input.mouse_button_state == 0)
    {
        set_element_with_capture(ui, alia_routable_element_id{});
        ui.input.dragging = false;
    }
}

void
process_double_click(
    ui_system& ui, vec2f const& position, button button, key_modifiers mods)
{
    auto target = get_mouse_target(ui);
    if (alia_routable_element_id_is_valid(target))
    {
        alia_event event = alia_make_double_click_event(
            {.button = alia_button_t(button),
             .mods = raw_code(mods),
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(ui, event, target);
    }
    ui.input.mouse_button_state |= 1 << unsigned(button);
    ui.input.keyboard_interaction = false;
}

void
process_scroll(ui_system& ui, vec2f const& delta)
{
    alia_event hit_test_event = alia_make_wheel_hit_test_event(
        {.x = ui.input.mouse_position.x, .y = ui.input.mouse_position.y});
    dispatch_event(ui, hit_test_event);
    if (alia_element_id_is_valid(hit_test_event.wheel_hit_test.result.element))
    {
        alia_event wheel_event = alia_make_wheel_event({.delta = delta});
        dispatch_targeted_event(
            ui, wheel_event, hit_test_event.wheel_hit_test.result);
        refresh_system(ui);
    }
}

#if 0

bool
process_focused_key_press(ui_system& ui, modded_key const& info)
{
    alia_event event = alia_make_key_press_event(
        {.mods = raw_code(info.mods), .key = info.key});
    dispatch_targeted_event(ui, event, ui.input.widget_with_focus);
    if (event.key_press.acknowledged)
        refresh_system(ui);
    return event.key_press.acknowledged;
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
