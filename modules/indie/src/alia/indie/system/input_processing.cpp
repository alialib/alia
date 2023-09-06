#include <alia/indie/system/input_processing.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/delivery.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/system/api.hpp>

namespace alia { namespace indie {

namespace {

std::shared_ptr<widget>
get_mouse_target(system& ui)
{
    // The widget with mouse capture takes precedence as the target for mouse
    // events.
    std::shared_ptr<widget> target = ui.input.widget_with_capture.lock();
    // But if no widget has capture, send events to the widget under the mouse.
    if (!target)
        target = ui.input.hot_widget.lock();
    return target;
}

} // namespace

void
process_mouse_motion(system& ui, vector<2, double> const& position)
{
    if (!ui.input.mouse_inside_window || ui.input.mouse_position != position)
    {
        mouse_motion_event event{
            {{}, input_event_type::MOUSE_MOTION}, position};
        deliver_input_event(ui, get_mouse_target(ui), event);

        ui.input.mouse_position = position;
        ui.input.mouse_inside_window = true;

        if (ui.input.mouse_button_state != 0)
            ui.input.dragging = true;
    }
}

void
process_mouse_loss(system& ui)
{
    ui.input.mouse_inside_window = false;
}

void
process_mouse_press(system& ui, mouse_button button, key_modifiers)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        mouse_button_event event{{{}, input_event_type::MOUSE_PRESS}, button};
        deliver_input_event(ui, target, event);
    }
    else
    {
        clear_focus(ui);
    }
    ui.input.mouse_button_state |= 1 << mouse_button_code(button);
    ui.input.keyboard_interaction = false;
}

void
process_mouse_release(system& ui, mouse_button button)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        mouse_button_event event{
            {{}, input_event_type::MOUSE_RELEASE}, button};
        deliver_input_event(ui, target, event);
    }
    ui.input.mouse_button_state &= ~(1 << int(button));
    if (ui.input.mouse_button_state == 0)
    {
        set_widget_with_capture(ui, external_widget_handle());
        ui.input.dragging = false;
    }
}

void
process_double_click(system& ui, mouse_button button)
{
    auto target = get_mouse_target(ui);
    if (target)
    {
        mouse_button_event event{{{}, input_event_type::DOUBLE_CLICK}, button};
        deliver_input_event(ui, target, event);
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

bool
process_focused_key_press(system& ui, modded_key const& info)
{
    key_event event{{{}, input_event_type::KEY_PRESS}, info};
    auto target = ui.input.widget_with_focus.lock();
    deliver_input_event(ui, target, event);
    return event.acknowledged;
}

bool
process_background_key_press(system& /*ui*/, modded_key const& /*info*/)
{
    // key_event event{{{}, input_event_type::DOUBLE_CLICK}, info};
    // key_event e(BACKGROUND_KEY_PRESS_EVENT, time, info);
    // issue_event(ui, e);
    // if (!e.acknowledged && info.code == input_event_type::KEY_PRESS)
    // {
    //     if (info.mods == KMOD_SHIFT)
    //     {
    //         regress_focus(ui);
    //         e.acknowledged = true;
    //     }
    //     else if (info.mods == 0)
    //     {
    //         advance_focus(ui);
    //         e.acknowledged = true;
    //     }
    // }
    // return e.acknowledged;
    return false;
}

bool
process_focused_key_release(system& ui, modded_key const& info)
{
    key_event event{{{}, input_event_type::KEY_RELEASE}, info};
    auto target = ui.input.widget_with_focus.lock();
    deliver_input_event(ui, target, event);
    return event.acknowledged;
}
bool
process_key_press(system& ui, modded_key const& info)
{
    ui.input.keyboard_interaction = true;
    return process_focused_key_press(ui, info);
}

bool
process_key_release(system& ui, modded_key const& info)
{
    ui.input.keyboard_interaction = true;
    return process_focused_key_release(ui, info);
}

}} // namespace alia::indie
