#include "alia/core/flow/top_level.hpp"
#include <alia/ui/system/input_processing.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/hit_testing.hpp>

namespace alia {

namespace {

external_element_id
get_mouse_target(ui_system& ui)
{
    // If no widget has capture, send events to the widget under the mouse.
    return ui.input.element_with_capture ? ui.input.element_with_capture
                                         : ui.input.hot_element;
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
            ui, event, get_mouse_target(ui).component, MOUSE_MOTION_EVENT);

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
        dispatch_targeted_event(
            ui, event, target.component, MOUSE_PRESS_EVENT);
    }
    else
    {
        clear_focus(ui);
    }
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
        dispatch_targeted_event(
            ui, event, target.component, MOUSE_RELEASE_EVENT);
    }
    ui.input.mouse_button_state &= ~(1 << int(button));
    if (ui.input.mouse_button_state == 0)
    {
        set_element_with_capture(ui, external_element_id());
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
        dispatch_targeted_event(
            ui, event, target.component, DOUBLE_CLICK_EVENT);
    }
    ui.input.mouse_button_state |= 1 << int(button);
    ui.input.keyboard_interaction = false;
}

// void
// process_wheel_movement(ui_system& ui, ui_time_type time, float movement)
// {
//     // First determine who should receive the event.
//     routable_widget_id target;
//     if (is_valid(ui.overlay_id))
//     {
//         wheel_hit_test_event hit_test;
//         hit_test.category = OVERLAY_CATEGORY;
//         hit_test.type = OVERLAY_WHEEL_HIT_TEST_EVENT;
//         issue_targeted_event(ui, hit_test, ui.overlay_id);
//         if (is_valid(hit_test.id))
//             target = hit_test.id;
//     }
//     if (!is_valid(target))
//     {
//         wheel_hit_test_event hit_test;
//         issue_event(ui, hit_test);
//         target = hit_test.id;
//     }
//     // Now dispatch it.
//     if (is_valid(target))
//     {
//         mouse_wheel_event event(time, target.id, movement);
//         issue_targeted_event(ui, event, target);
//     }
// }

void
process_scroll(ui_system& /*ui*/, vector<2, double> const& /*delta*/)
{
    // TODO
    // wheel_hit_test hit_test;
    // ui.root_widget->hit_test(hit_test, ui.input.mouse_position);
    // if (hit_test.result)
    // {
    //     scroll_event event{{{}, ui_event_type::SCROLL}, delta};
    //     deliver_input_event(ui, hit_test.result->widget, event);
    // }
}

bool
process_focused_key_press(ui_system& ui, modded_key const& info)
{
    key_event event{{}, info};
    dispatch_targeted_event(
        ui, event, ui.input.element_with_focus.component, KEY_PRESS_EVENT);
    return event.acknowledged;
}

bool
process_background_key_press(ui_system& ui, modded_key const& info)
{
    key_event event{{}, info};
    dispatch_event(ui, event, BACKGROUND_KEY_PRESS_EVENT);
    return event.acknowledged;
}

bool
process_focused_key_release(ui_system& ui, modded_key const& info)
{
    key_event event{{}, info};
    dispatch_targeted_event(
        ui, event, ui.input.element_with_focus.component, KEY_RELEASE_EVENT);
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
