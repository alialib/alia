#include <alia/ui/system/input_processing.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/core/flow/top_level.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/regions.hpp>

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
process_mouse_motion(ui_system& ui, vector<2, double> const& position)
{
    if (!ui.input.mouse_inside_window || ui.input.mouse_position != position)
    {
        mouse_motion_event event;
        // TODO: Use constructor
        event.position = position;
        dispatch_targeted_event(
            ui, event, get_mouse_target(ui), MOUSE_MOTION_EVENT);
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
process_mouse_press(ui_system& ui, mouse_button button, key_modifiers)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        mouse_button_event event;
        // TODO: Use constructor
        event.button = button;
        dispatch_targeted_event(ui, event, target, MOUSE_PRESS_EVENT);
        refresh_system(ui);
    }
    else
    {
        clear_focus(ui);
    }
    ui.input.last_mouse_press_time = ui.tick_count;
    ui.input.mouse_button_state |= 1 << mouse_button_code(button);
    ui.input.keyboard_interaction = false;
}

void
process_mouse_release(ui_system& ui, mouse_button button)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        mouse_button_event event;
        // TODO: Use constructor
        event.button = button;
        dispatch_targeted_event(ui, event, target, MOUSE_RELEASE_EVENT);
        refresh_system(ui);
    }
    ui.input.mouse_button_state &= ~(1 << int(button));
    if (ui.input.mouse_button_state == 0)
    {
        set_widget_with_capture(ui, routable_widget_id());
        ui.input.dragging = false;
    }
}

void
process_double_click(ui_system& ui, mouse_button button)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        mouse_button_event event;
        // TODO: Use constructor
        event.button = button;
        dispatch_targeted_event(ui, event, target, DOUBLE_CLICK_EVENT);
    }
    ui.input.mouse_button_state |= 1 << int(button);
    ui.input.keyboard_interaction = false;
}

void
process_scroll(ui_system& ui, vector<2, double> const& delta)
{
    wheel_hit_test_event hit_test{ui.input.mouse_position};
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

} // namespace alia
