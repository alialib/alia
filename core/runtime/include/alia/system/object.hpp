#pragma once

// #include <alia/core/system/internals.hpp>
// #include <alia/core/timing/ticks.hpp>
#include <alia/animation/flares.hpp>
#include <alia/animation/transitions.hpp>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/geometry.hpp>
#include <alia/ids.hpp>
#include <alia/input/constants.hpp>
#include <alia/layout/system.hpp>
// #include <alia/system/os_interface.hpp>
// #include <alia/system/window_interface.hpp>
#include <alia/theme.hpp>

#include <functional>

namespace alia {

struct window_input_state
{
    // Is the mouse inside the window associated with this UI?
    bool mouse_inside_window = false;

    // the state of the mouse buttons (one bit per button)
    unsigned mouse_button_state = 0;

    // the raw mouse position inside the window
    vec2 mouse_position;

    // the tick count corresponding to the last press of each mouse button
    nanosecond_count last_mouse_press_time[ALIA_MAX_SUPPORTED_MOUSE_BUTTONS];

    // the element that the mouse is over
    alia_routable_element_id hot_element;

    // the element that has the mouse captured - Note that this isn't
    // necessarily the same as the hot_element.
    alia_routable_element_id element_with_capture;

    // the element that has the keyboard focus
    alia_routable_element_id element_with_focus;

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
    nanosecond_count hover_start_time;

    // the mouse cursor that's currently set for our window
    cursor current_cursor = cursor::DEFAULT;
};

struct animation_tracking_system
{
    // active transitions
    // transition_animation_map transitions;
    // active flare groups
    flare_map flares;
};

struct ui_system
{
    animation_tracking_system animation;

    std::function<void(context&)> controller;

    // alia__shared_ptr<alia::surface> surface;
    vec2 surface_size;
    vec2 ppi;

    float magnification = 1;

    // std::shared_ptr<os_interface> os;

    // std::shared_ptr<window_interface> window;

    window_input_state input;

    layout_system layout;

    // the current time, as a millisecond tick counter with an arbitrary start
    // point and the possibility of wraparound
    nanosecond_count tick_count = 0;

    theme_colors theme;

    // routable_widget_id overlay_id;

    // std::vector<widget_visibility_request> pending_visibility_requests;

    // This prevents timer requests from being serviced in the same frame that
    // they're requested and thus throwing the event handler into a loop.
    // counter_type timer_event_counter;

    // int last_refresh_duration;

    // tooltip_state tooltip;
};

void
initialize(ui_system& system, vec2 surface_size);

} // namespace alia
