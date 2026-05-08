#include <alia/abi/ui/system/input_processing.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/system/work.h>
#include <alia/ui/system/object.h>

using namespace alia::operators;

extern "C" {

void
alia_ui_process_mouse_motion(alia_ui_system* ui, alia_vec2f position)
{
    if (!ui->input.mouse_inside_window
        || !alia_vec2f_equal(ui->input.mouse_position, position))
    {
        alia_event event
            = alia_make_mouse_motion_event({.x = position.x, .y = position.y});
        alia_ui_enqueue_event(ui, &event);
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
    alia_event event = alia_make_mouse_press_event(
        {.button = button, .mods = mods, .x = position.x, .y = position.y});
    alia_ui_enqueue_event(ui, &event);
}

void
alia_ui_process_mouse_release(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    alia_event event = alia_make_mouse_release_event(
        {.button = button, .mods = mods, .x = position.x, .y = position.y});
    alia_ui_enqueue_event(ui, &event);
}

void
alia_ui_process_double_click(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods)
{
    alia_event event = alia_make_double_click_event(
        {.button = button, .mods = mods, .x = position.x, .y = position.y});
    alia_ui_enqueue_event(ui, &event);
}

void
alia_ui_process_scroll(alia_ui_system* ui, alia_vec2f delta)
{
    alia_event event = alia_make_scroll_input_event({.delta = delta});
    alia_ui_enqueue_event(ui, &event);
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
    alia_ui_enqueue_event(ui, &event);
    // Acknowledgement is determined when `alia_ui_system_update` drains the
    // queue.
    return false;
}

bool
alia_ui_process_background_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_background_key_press_event(
        {.code = key, .mods = mods, .acknowledged = false});
    alia_ui_enqueue_event(ui, &event);
    return false;
}

bool
alia_ui_process_focused_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_key_release_event(
        {.code = key, .mods = mods, .acknowledged = false});
    alia_ui_enqueue_event(ui, &event);
    return false;
}

bool
alia_ui_process_background_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    alia_event event = alia_make_background_key_release_event(
        {.code = key, .mods = mods, .acknowledged = false});
    alia_ui_enqueue_event(ui, &event);
    return false;
}

bool
alia_ui_process_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    ui->input.keyboard_interaction = true;
    alia_event event = alia_make_key_press_event(
        {.code = key, .mods = mods, .acknowledged = false});
    alia_ui_enqueue_event(ui, &event);
    return false;
}

bool
alia_ui_process_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods)
{
    ui->input.keyboard_interaction = true;
    alia_event event = alia_make_key_release_event(
        {.code = key, .mods = mods, .acknowledged = false});
    alia_ui_enqueue_event(ui, &event);
    return false;
}

void
alia_ui_process_focus_loss(alia_ui_system* ui)
{
    (void) ui;
}

void
alia_ui_process_focus_gain(alia_ui_system* ui)
{
    (void) ui;
}

} // extern "C"
