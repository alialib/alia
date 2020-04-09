#ifndef ALIA_SYSTEM_INTERNALS_HPP
#define ALIA_SYSTEM_INTERNALS_HPP

#include <functional>

#include <alia/context/interface.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/timing/scheduler.hpp>
#include <alia/timing/ticks.hpp>

namespace alia {

// Get the default implementation of the millisecond tick counter.
// (This uses std::chrono::steady_clock.)
millisecond_count
get_default_tick_count();

struct external_interface
{
    // Get the current value of the system's millisecond tick counter.
    // (The default implementation uses std::chrono::steady_clock.)
    virtual millisecond_count
    get_tick_count() const
    {
        return get_default_tick_count();
    }

    // alia calls this every frame when an animation is in progress that's using
    // alia's internal animation timing system.
    //
    // If this system isn't already refreshing continuously, this requests that
    // it refresh again within a reasonable animation time frame.
    //
    // (You can also call system_needs_refresh (see below) to test if a refresh
    // is necessary.)
    //
    virtual void
    schedule_animation_refresh()
    {
    }

    // Schedule a timer event to be delivered to a specific component at some
    // future time.
    //
    // :id is the ID of the component that the event should be delivered to.
    //
    // :time is the tick count at which the event should be delivered.
    //
    // alia provides an internal system for tracking outstanding requests for
    // timer events.
    // TODO: Finish documenting this!
    virtual void
    schedule_timer_event(routable_node_id component, millisecond_count time)
    {
        // TODO!
    }
};

struct system
{
    data_graph data;
    std::function<void(context)> controller;
    bool refresh_needed = false;
    external_interface* external = nullptr;
    timer_event_scheduler timing;
};

} // namespace alia

#endif
