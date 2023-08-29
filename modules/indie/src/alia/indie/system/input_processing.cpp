#include <alia/indie/system/input_processing.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/system/api.hpp>

namespace alia { namespace indie {

inline external_component_id
get_mouse_target(system& ui)
{
    return ui.input.widget_with_capture ? ui.input.id_with_capture
                                        : ui.input.hot_id;
}

void
process_mouse_motion(system& ui, vector<2, double> const& position)
{
    if (!ui.input.mouse_inside_window || ui.input.mouse_position != position)
    {
        mouse_motion_event event{
            {{}, input_event_type::MOUSE_MOTION}, position};
        dispatch_targeted_event(ui, event, get_mouse_target(ui));

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
    if (is_valid(target))
    {
        mouse_button_event event{{{}, input_event_type::MOUSE_PRESS}, button};
        dispatch_targeted_event(ui, event, target);
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
    mouse_button_event event{{{}, input_event_type::MOUSE_RELEASE}, button};
    dispatch_targeted_event(ui, event, get_mouse_target(ui));
    ui.input.mouse_button_state &= ~(1 << int(button));
    if (ui.input.mouse_button_state == 0)
    {
        set_component_with_capture(ui, null_component_id);
        ui.input.dragging = false;
    }
}

void
process_double_click(system& ui, mouse_button button)
{
    mouse_button_event event{{{}, input_event_type::DOUBLE_CLICK}, button};
    dispatch_targeted_event(ui, event, get_mouse_target(ui));
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

}} // namespace alia::indie
