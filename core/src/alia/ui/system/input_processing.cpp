#include <alia/system/input_processing.hpp>

#include <alia/abi/base/geometry.h>
#include <alia/flow/dispatch.hpp>
#include <alia/impl/events.hpp>
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
process_mouse_motion(ui_system& ui, alia_vec2f position)
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
    ui_system& ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    auto target = get_mouse_target(ui);
    if (alia_routable_element_id_is_valid(target))
    {
        alia_event event = alia_make_mouse_press_event(
            {.button = button,
             .mods = mods,
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
    ui_system& ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    auto target = get_mouse_target(ui);
    if (alia_routable_element_id_is_valid(target))
    {
        alia_event event = alia_make_mouse_release_event(
            {.button = button,
             .mods = mods,
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
    ui_system& ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    auto target = get_mouse_target(ui);
    if (alia_routable_element_id_is_valid(target))
    {
        alia_event event = alia_make_double_click_event(
            {.button = button,
             .mods = mods,
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(ui, event, target);
    }
    ui.input.mouse_button_state |= 1 << unsigned(button);
    ui.input.keyboard_interaction = false;
}

void
process_scroll(ui_system& ui, alia_vec2f delta)
{
    alia_event hit_test_event = alia_make_wheel_hit_test_event(
        {.x = ui.input.mouse_position.x, .y = ui.input.mouse_position.y});
    dispatch_event(ui, hit_test_event);
    if (alia_element_id_is_valid(
            as_wheel_hit_test_event(hit_test_event).result.element))
    {
        alia_event wheel_event = alia_make_wheel_event({.delta = delta});
        dispatch_targeted_event(
            ui, wheel_event, as_wheel_hit_test_event(hit_test_event).result);
        refresh_system(ui);
    }
}

void
set_focus(ui_system& sys, routable_widget_id widget)
{
    // TODO: Implement this logic once there is an internal event queue.

    // bool different = sys.input.widget_with_focus.id != widget.id;
    // if (different && sys.input.widget_with_focus)
    // {
    //         // A lot of code likes to call set_focus in response to events,
    //         // which means that the following FOCUS_LOSS_EVENT could end up
    //         // being invoked on a UI state that hasn't seen a refresh event
    //         // yet, so do a refresh here just to be safe.
    //         refresh_system(sys);
    //
    //     focus_notification_event event;
    //     dispatch_targeted_event(
    //         sys, event, sys.input.widget_with_focus, FOCUS_LOSS_EVENT);
    //     refresh_system(sys);
    // }

    // sys.input.widget_with_focus = widget;

    // // It's possible to have widgets that appear based on whether or not
    // // another widget has the focus, so we need to refresh here.
    // refresh_system(sys);

    // TODO
    // if (different && widget)
    // {
    //     // Make the new widget visible.
    //     {
    //         auto widget = element.widget.lock();
    //         if (widget && widget->parent)
    //         {
    //             widget->parent->reveal_region(region_reveal_request{
    //                 layout_box(transform_box(
    //                     widget->transformation(),
    //                     box2d(widget->bounding_box()))),
    //                 false,
    //                 false});
    //         }
    //     }

    //     focus_notification_event event{{{}, ui_event_type::FOCUS_GAIN}};
    //     deliver_input_event(sys, element.widget, event);
    // }
}

#if 0

bool
process_focused_key_press(
    ui_system& ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_key_press_event({.mods = mods, .key = key});
    dispatch_targeted_event(ui, event, ui.input.widget_with_focus);
    if (event.key_press.acknowledged)
        refresh_system(ui);
    return event.key_press.acknowledged;
}

bool
process_background_key_press(
    ui_system& ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_key_press_event({.mods = mods, .key = key});
    dispatch_event(ui, event, BACKGROUND_KEY_PRESS_EVENT);
    if (event.acknowledged)
        refresh_system(ui);
    return event.acknowledged;
}

bool
process_focused_key_release(ui_system& ui, modded_key info)
{
    key_event event{{}, info};
    dispatch_targeted_event(
        ui, event, ui.input.widget_with_focus, KEY_RELEASE_EVENT);
    if (event.acknowledged)
        refresh_system(ui);
    return event.acknowledged;
}
bool
process_key_press(ui_system& ui, modded_key info)
{
    ui.input.keyboard_interaction = true;
    return process_focused_key_press(ui, info)
        || process_background_key_press(ui, info);
}

bool
process_key_release(ui_system& ui, modded_key info)
{
    ui.input.keyboard_interaction = true;
    return process_focused_key_release(ui, info);
}

#endif

} // namespace alia
