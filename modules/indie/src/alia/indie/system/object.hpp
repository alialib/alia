#ifndef ALIA_INDIE_SYSTEM_INTERNAL_DEFINITION_HPP
#define ALIA_INDIE_SYSTEM_INTERNAL_DEFINITION_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/system/internals.hpp>
#include <alia/core/timing/ticks.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/internals.hpp>
#include <alia/indie/system/os_interface.hpp>
#include <alia/indie/system/window_interface.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

struct widget;

struct window_input_state
{
    // Is the mouse inside the window associated with this UI?
    bool mouse_inside_window = false;

    // the state of the mouse buttons (one bit per button)
    unsigned mouse_button_state = 0;

    // the raw mouse position inside the window
    vector<2, double> mouse_position;

    // the widget that the mouse is over
    external_widget_handle hot_widget;

    // the widget that has the mouse captured - Note that this isn't
    // necessarily the same as the hot_widget.
    external_widget_handle widget_with_capture;

    // the widget that has the keyboard focus
    external_widget_handle widget_with_focus;

    // Is the user currently dragging the mouse (with a button pressed)?
    bool dragging = false;

    // Does the window have focus?
    bool window_has_focus = true;

    // Is the user currently interacting with the UI via the keyboard? - This
    // is used as a hint to display focus indicators.
    bool keyboard_interaction = false;

    // If the mouse is hovering over a widget (identified by hot_widget), this
    // is the time at which the hovering started. Note that hovering is only
    // possible if no widget has captured the mouse.
    millisecond_count hover_start_time;

    // the mouse cursor that's currently set for our window
    mouse_cursor current_mouse_cursor = mouse_cursor::DEFAULT;
};

struct system : alia::typed_system<indie::vanilla_context>
{
    std::function<void(indie::context)> controller;
    void invoke_controller(indie::vanilla_context) override;

    layout_system layout;

    widget* root_widget;

    // alia__shared_ptr<alia::surface> surface;
    vector<2, unsigned> surface_size;
    // vector<2,float> ppi;

    std::shared_ptr<os_interface> os;

    std::shared_ptr<window_interface> window;

    window_input_state input;

    // ui_style style;

    // routable_widget_id overlay_id;

    // std::vector<widget_visibility_request> pending_visibility_requests;

    // This prevents timer requests from being serviced in the same frame that
    // they're requested and thus throwing the event handler into a loop.
    // counter_type timer_event_counter;

    // int last_refresh_duration;

    // tooltip_state tooltip;
};

}} // namespace alia::indie

#endif
