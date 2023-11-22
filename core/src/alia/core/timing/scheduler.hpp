#ifndef ALIA_CORE_TIMING_SCHEDULER_HPP
#define ALIA_CORE_TIMING_SCHEDULER_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/timing/ticks.hpp>

#include <vector>

// This file defines the default implementation for tracking timer events.

namespace alia {

struct timer_event_request
{
    millisecond_count trigger_time;
    std::function<void()> callback;
    unsigned frame_issued;
};

struct timer_event_scheduler
{
    std::vector<timer_event_request> requests;
    unsigned frame_counter = 0;
};

// Schedule an event.
void
schedule_callback(
    timer_event_scheduler& scheduler,
    std::function<void()> callback,
    millisecond_count time);

// Issue any events that are ready to be issued.
void
issue_ready_events(
    timer_event_scheduler& scheduler,
    millisecond_count now,
    function_view<void(std::function<void()> const&)> issue);

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
