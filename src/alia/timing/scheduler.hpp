#ifndef ALIA_TIMING_SCHEDULER_HPP
#define ALIA_TIMING_SCHEDULER_HPP

#include <alia/flow/events.hpp>
#include <alia/timing/ticks.hpp>

#include <vector>

// This file defines the default implementation for tracking timer events.

namespace alia {

struct timer_event_request
{
    millisecond_count trigger_time;
    routable_component_id component;
    unsigned frame_issued;
};

struct timer_event_scheduler
{
    std::vector<timer_event_request> requests;
    unsigned frame_counter = 0;
};

// Schedule an event.
void
schedule_event(
    timer_event_scheduler& scheduler,
    routable_component_id component,
    millisecond_count time);

// Issue any events that are ready to be issued.
void
issue_ready_events(
    timer_event_scheduler& scheduler,
    millisecond_count now,
    function_view<void(
        routable_component_id component, millisecond_count time)> const& issue);

// Are there any scheduled events?
bool
has_scheduled_events(timer_event_scheduler const& scheduler);

// Get the time until the next scheduled event.
// The behavior is undefined if there are no scheduled events.
millisecond_count
get_time_until_next_event(
    timer_event_scheduler& scheduler, millisecond_count now);

} // namespace alia

#endif
