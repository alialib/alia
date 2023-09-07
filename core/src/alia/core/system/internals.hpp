#ifndef ALIA_CORE_SYSTEM_INTERNALS_HPP
#define ALIA_CORE_SYSTEM_INTERNALS_HPP

#include <functional>
#include <memory>

#include <alia/core/context/interface.hpp>
#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/flow/top_level.hpp>
#include <alia/core/timing/scheduler.hpp>
#include <alia/core/timing/ticks.hpp>

namespace alia {

struct external_interface
{
    virtual ~external_interface()
    {
    }

    // Get the current value of the system's millisecond tick counter.
    // (The default implementation uses std::chrono::steady_clock.)
    virtual millisecond_count
    get_tick_count() const;

    // alia calls this every frame when an animation is in progress that's
    // using alia's internal animation timing system.
    //
    // If this system isn't already refreshing continuously, this requests that
    // it refresh again within a reasonable animation time frame.
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
    virtual void
    schedule_timer_event(
        external_component_id component, millisecond_count time)
        = 0;

    // alia calls this when it needs to schedule an update asynchronously.
    //
    // The system should schedule the given update function to run on the UI
    // thread as soon as possible.
    //
    virtual void
    schedule_asynchronous_update(std::function<void()> update)
        = 0;
};

struct untyped_system;

// alia provides an internal system for tracking outstanding requests for timer
// events. If this system is continuously updating anyway, you can use this
// default implementation and call `process_internal_timing_events` once per
// frame to handle timing events. (See below.)
//
struct default_external_interface : external_interface
{
    untyped_system& owner;

    default_external_interface(untyped_system& owner) : owner(owner)
    {
    }

    virtual void
    schedule_animation_refresh() override
    {
    }

    virtual void
    schedule_timer_event(
        external_component_id component, millisecond_count time) override;

    virtual void
    schedule_asynchronous_update(std::function<void()> update) override;
};

// TODO: Clean this up.

struct untyped_system : noncopyable
{
    data_graph data;
    bool refresh_needed = false;
    counter_type refresh_counter = 0;
    std::unique_ptr<external_interface> external;
    timer_event_scheduler scheduler;
    component_container_ptr root_component;
    std::function<void(std::exception_ptr)> error_handler;

    virtual void
    create_core_context_and_invoke_controller(
        event_traversal& event, data_traversal& data, timing_subsystem& timing)
        = 0;
};

template<class Context>
struct typed_system : untyped_system
{
    typedef Context context_type;

    void
    create_core_context_and_invoke_controller(
        event_traversal& event,
        data_traversal& dt,
        timing_subsystem& timing) override
    {
        typename Context::contents_type::storage_type storage;
        auto ctx = make_context(&storage, *this, event, dt, timing);
        scoped_component_container root(ctx, &this->root_component);
        this->invoke_controller(ctx);
    }

    virtual void
    invoke_controller(Context ctx)
        = 0;
};

template<class Context>
void
initialize_core_system(
    typed_system<Context>& sys, external_interface* external = nullptr)
{
    if (external)
        sys.external.reset(external);
    else
        sys.external = std::make_unique<default_external_interface>(sys);
    sys.root_component = std::make_shared<component_container>();
}

struct system : typed_system<context>
{
    std::function<void(context)> controller;

    virtual void
    invoke_controller(context ctx)
    {
        controller(ctx);
    }
};

void
initialize_standalone_system(
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
template<class System>
void
process_internal_timing_events(System& sys, millisecond_count now)
{
    issue_ready_events(
        sys.scheduler,
        now,
        [&](external_component_id component, millisecond_count trigger_time) {
            timer_event event;
            event.trigger_time = trigger_time;
            dispatch_targeted_event(sys, event, component);
        });
}

} // namespace alia

#endif