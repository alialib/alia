#pragma once

// #include <alia/core/system/internals.hpp>
// #include <alia/core/timing/ticks.hpp>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/input/state.h>
#include <alia/base/stack.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/animation.h>
#include <alia/kernel/flow/ids.h>
#include <alia/kernel/substrate.h>
#include <alia/ui/layout/system.h>
// #include <alia/system/os_interface.hpp>
// #include <alia/system/window_interface.hpp>
#include <alia/abi/ui/palette.h>
#include <alia/ui/drawing.h>

#include <functional>

extern "C" {

struct alia_ui_system
{
    alia::animation_system animation;

    alia_draw_system draw;

    std::function<void(alia::context&)> controller;

    // alia__shared_ptr<alia::surface> surface;
    alia_vec2i surface_size;
    float dpi;
    float magnification = 1;

    // std::shared_ptr<os_interface> os;

    // std::shared_ptr<window_interface> window;

    alia_input_state input;

    alia_layout_system layout;

    alia_substrate_system substrate;
    alia_arena substrate_discovery_arena;

    // the current time, as a millisecond tick counter with an arbitrary start
    // point and the possibility of wraparound
    alia_nanosecond_count tick_count = 0;

    alia_palette palette;

    alia_stack stack;

    alia_arena scratch;

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
