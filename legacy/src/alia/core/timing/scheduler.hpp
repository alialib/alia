#ifndef ALIA_CORE_TIMING_SCHEDULER_HPP
#define ALIA_CORE_TIMING_SCHEDULER_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/timing/ticks.hpp>

#include <vector>

// This file defines the default implementation for tracking timer events.

namespace alia {

struct callback_request
{
    millisecond_count trigger_time;
    std::function<void()> callback;
    unsigned frame_issued;
};

struct callback_scheduler
{
    std::vector<callback_request> requests;
    unsigned frame_counter = 0;
};

// Schedule an event.
void
schedule_callback(
    callback_scheduler& scheduler,
    std::function<void()> callback,
    millisecond_count time);

// Invoke any callbacks that are due to be called.
void
invoke_ready_callbacks(
    callback_scheduler& scheduler,
    millisecond_count now,
    function_view<void(std::function<void()> const&, millisecond_count)> const&
        invoker);

// Are there any scheduled callbacks?
bool
has_scheduled_callbacks(callback_scheduler const& scheduler);

// Get the time until the next scheduled callback.
// The behavior is undefined if there are no scheduled callbacks.
millisecond_count
time_until_next_callback(callback_scheduler& scheduler, millisecond_count now);

} // namespace alia

#endif
