#include <alia/abi/ui/system/input_processing.h>

#include <alia/abi/base/geometry.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/ui/system/object.h>

// TODO: API shouldn't be needed here.
#include <alia/ui/system/internal_api.h>

using namespace alia::operators;
using namespace alia;

namespace {

alia_element_id
get_mouse_target(alia_ui_system* ui)
{
    // If no widget has capture, send events to the widget under the mouse.
    return alia_element_id_is_valid(ui->input.element_with_capture)
             ? ui->input.element_with_capture
             : ui->input.hot_element;
}

} // namespace

extern "C" {

void
alia_ui_process_mouse_motion(alia_ui_system* ui, alia_vec2f position)
{
    if (!ui->input.mouse_inside_window
        || !alia_vec2f_equal(ui->input.mouse_position, position))
    {
        alia_event event
            = alia_make_mouse_motion_event({.x = position.x, .y = position.y});
        dispatch_targeted_event(*ui, event, get_mouse_target(ui));
        refresh_system(*ui);

        ui->input.mouse_position = position;
        ui->input.mouse_inside_window = true;

        if (ui->input.mouse_button_state != 0)
            ui->input.dragging = true;
    }
}

void
alia_ui_process_mouse_loss(alia_ui_system* ui)
{
    ui->input.mouse_inside_window = false;
}

void
alia_ui_process_mouse_press(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    auto target = get_mouse_target(ui);
    if (alia_element_id_is_valid(target))
    {
        alia_event event = alia_make_mouse_press_event(
            {.button = button,
             .mods = mods,
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(*ui, event, target);
        refresh_system(*ui);
    }
    else
    {
        // TODO
        // clear_focus(ui);
    }
    ui->input.last_mouse_press_time[unsigned(button)] = ui->tick_count;
    ui->input.mouse_button_state |= 1 << unsigned(button);
    ui->input.keyboard_interaction = false;
}

void
alia_ui_process_mouse_release(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    auto target = get_mouse_target(ui);
    if (alia_element_id_is_valid(target))
    {
        alia_event event = alia_make_mouse_release_event(
            {.button = button,
             .mods = mods,
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(*ui, event, target);
        refresh_system(*ui);
    }
    ui->input.mouse_button_state &= ~(1 << unsigned(button));
    if (ui->input.mouse_button_state == 0)
    {
        set_element_with_capture(*ui, alia_element_id{});
        ui->input.dragging = false;
    }
}

void
alia_ui_process_double_click(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    auto target = get_mouse_target(ui);
    if (alia_element_id_is_valid(target))
    {
        alia_event event = alia_make_double_click_event(
            {.button = button,
             .mods = mods,
             .x = position.x,
             .y = position.y});
        dispatch_targeted_event(*ui, event, target);
    }
    ui->input.mouse_button_state |= 1 << unsigned(button);
    ui->input.keyboard_interaction = false;
}

void
alia_ui_process_scroll(alia_ui_system* ui, alia_vec2f delta)
{
    alia_event hit_test_event = alia_make_wheel_hit_test_event(
        {.x = ui->input.mouse_position.x, .y = ui->input.mouse_position.y});
    dispatch_event(*ui, hit_test_event);
    if (alia_element_id_is_valid(
            as_wheel_hit_test_event(hit_test_event).result))
    {
        alia_event wheel_event = alia_make_wheel_event({.delta = delta});
        dispatch_targeted_event(
            *ui, wheel_event, as_wheel_hit_test_event(hit_test_event).result);
        refresh_system(*ui);
    }
}

void
alia_ui_set_focus(alia_ui_system* ui, alia_element_id widget)
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

bool
alia_ui_process_focused_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_key_press_event(
        {.code = key, .mods = mods, .acknowledged = false});
    dispatch_targeted_event(*ui, event, ui->input.element_with_focus);
    bool acknowledged = as_key_press_event(event).acknowledged;
    // TODO: Figure out the semantics of this.
    if (acknowledged)
        refresh_system(*ui);
    return acknowledged;
}

bool
alia_ui_process_background_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_background_key_press_event(
        {.code = key, .mods = mods, .acknowledged = false});
    dispatch_event(*ui, event);
    bool acknowledged = as_background_key_press_event(event).acknowledged;
    // TODO: Figure out the semantics of this.
    if (acknowledged)
        refresh_system(*ui);
    return acknowledged;
}

bool
alia_ui_process_focused_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_key_release_event(
        {.code = key, .mods = mods, .acknowledged = false});
    dispatch_targeted_event(*ui, event, ui->input.element_with_focus);
    bool acknowledged = as_key_release_event(event).acknowledged;
    // TODO: Figure out the semantics of this.
    if (acknowledged)
        refresh_system(*ui);
    return acknowledged;
}

bool
alia_ui_process_background_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_background_key_release_event(
        {.code = key, .mods = mods, .acknowledged = false});
    dispatch_event(*ui, event);
    bool acknowledged = as_background_key_release_event(event).acknowledged;
    // TODO: Figure out the semantics of this.
    if (acknowledged)
        refresh_system(*ui);
    return acknowledged;
}

bool
alia_ui_process_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    ui->input.keyboard_interaction = true;
    return alia_ui_process_focused_key_press(ui, key, mods)
        || alia_ui_process_background_key_press(ui, key, mods);
}

bool
alia_ui_process_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    ui->input.keyboard_interaction = true;
    return alia_ui_process_focused_key_release(ui, key, mods)
        || alia_ui_process_background_key_release(ui, key, mods);
}

} // extern "C"
