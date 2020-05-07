#ifndef ALIA_SYSTEM_INTERNALS_HPP
#define ALIA_SYSTEM_INTERNALS_HPP

#include <functional>
#include <memory>

#include <alia/context/interface.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>
#include <alia/timing/scheduler.hpp>
#include <alia/timing/ticks.hpp>

namespace alia {

struct system;

struct external_interface
{
    virtual ~external_interface()
    {
    }

    // Get the current value of the system's millisecond tick counter.
    // (The default implementation uses std::chrono::steady_clock.)
    virtual millisecond_count
    get_tick_count() const = 0;

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
        = 0;

    // Schedule a timer event to be delivered to a specific component at some
    // future time.
    //
    // :id is the ID of the component that the event should be delivered to.
    //
    // :time is the tick count at which the event should be delivered.
    //
    // alia provides an internal system for tracking outstanding requests for
    // timer events. If this system is continuously updating anyway, you can
    // leave this unimplemented and call process_internal_timing_events
    // once per frame to handle timing events. (See below.)
    //
    virtual void
    schedule_timer_event(
        external_component_id component, millisecond_count time)
        = 0;
};

struct default_external_interface : external_interface
{
    system& owner;

    default_external_interface(system& owner) : owner(owner)
    {
    }

    virtual millisecond_count
    get_tick_count() const;

    void
    schedule_animation_refresh()
    {
    }

    void
    schedule_timer_event(
        external_component_id component, millisecond_count time);
};

struct system : noncopyable
{
    data_graph data;
    std::function<void(context)> controller;
    bool refresh_needed = false;
    std::unique_ptr<external_interface> external;
    timer_event_scheduler scheduler;
    routing_region_ptr root_region;
};

void
initialize_system(
    system& sys,
    std::function<void(context)> const& controller,
    external_interface* external = nullptr);

// timer event
struct timer_event : targeted_event
{
    millisecond_count trigger_time;
};

// If this system is using internal timer event scheduling, this will check for
// any events that are ready to be issued and issue them.
void
process_internal_timing_events(system& sys, millisecond_count now);

} // namespace alia

#endif
