#pragma once

// #include <alia/core/system/internals.hpp>
// #include <alia/core/timing/ticks.hpp>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/input/state.h>
#include <alia/animation/flares.hpp>
#include <alia/animation/transitions.hpp>
#include <alia/base/stack.h>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/ids.hpp>
#include <alia/ui/layout/system.h>
// #include <alia/system/os_interface.hpp>
// #include <alia/system/window_interface.hpp>
#include <alia/theme.hpp>

#include <functional>

namespace alia {

struct animation_tracking_system
{
    // active transitions
    // transition_animation_map transitions;
    // active flare groups
    flare_map flares;
};

} // namespace alia

extern "C" {

struct alia_ui_system
{
    alia::animation_tracking_system animation;

    std::function<void(alia::context&)> controller;

    // alia__shared_ptr<alia::surface> surface;
    alia_vec2f surface_size;
    alia_vec2f ppi;
    float magnification = 1;

    // std::shared_ptr<os_interface> os;

    // std::shared_ptr<window_interface> window;

    alia_input_state input;

    alia_layout_system layout;

    // the current time, as a millisecond tick counter with an arbitrary start
    // point and the possibility of wraparound
    alia_nanosecond_count tick_count = 0;

    alia::theme_colors theme;

    alia_stack stack;

    // alia::routable_widget_id overlay_id;

    // std::vector<alia::widget_visibility_request>
    // pending_visibility_requests;

    // This prevents timer requests from being serviced in the same frame that
    // they're requested and thus throwing the event handler into a loop.
    // alia::counter_type timer_event_counter;

    // int last_refresh_duration;

    // alia::tooltip_state tooltip;
};

} // extern "C"

namespace alia {

void
initialize_ui_system(alia_ui_system* system, alia_vec2f surface_size);

// TODO: cleanup function

} // namespace alia
