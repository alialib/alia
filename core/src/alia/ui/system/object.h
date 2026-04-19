#pragma once

// #include <alia/core/system/internals.hpp>
// #include <alia/core/timing/ticks.hpp>
#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/ui/input/state.h>
#include <alia/abi/ui/msdf.h>
#include <alia/base/stack.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/animation.h>
#include <alia/kernel/substrate.h>
#include <alia/ui/layout/system.h>
// #include <alia/system/os_interface.hpp>
// #include <alia/system/window_interface.hpp>
#include <alia/abi/ui/palette.h>
#include <alia/ui/drawing.h>

#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

extern "C" {

// A pending timer request queued by component code.
// Dispatch happens during `alia_ui_system_update`.
struct alia_ui_timer_request
{
    alia_element_id target;
    alia_nanosecond_count fire_time;
    // Used to prevent timers from being dispatched in the same update cycle
    // they were queued (avoids re-entrancy loops).
    uint64_t queued_in_cycle = 0;
};

struct alia_ui_timer_request_compare
{
    bool
    operator()(
        alia_ui_timer_request const& a, alia_ui_timer_request const& b) const
    {
        // Min-heap by fire_time.
        return a.fire_time > b.fire_time;
    }
};

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

    // optional MSDF text engine used by core widgets that render glyphs
    alia_msdf_text_engine* msdf_text_engine = nullptr;

    // pending timer events
    uint64_t timer_event_counter = 0;
    std::priority_queue<
        alia_ui_timer_request,
        std::vector<alia_ui_timer_request>,
        alia_ui_timer_request_compare>
        timer_requests;

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
