#pragma once

#include <alia/abi/kernel/object.h>
#include <alia/abi/kernel/routing.h>
#include <alia/base/memory.h>
#include <alia/kernel/animation.h>
#include <alia/kernel/substrate.h>

#include <queue>
#include <vector>

// a timer request queued by component code
typedef struct alia_kernel_timer_request
{
    alia_element_id target;
    alia_nanosecond_count fire_time;
    // cycle in which this request was queued
    uint64_t queued_in_cycle = 0;
} alia_kernel_timer_request;

struct alia_kernel_timer_request_compare
{
    bool
    operator()(
        alia_kernel_timer_request const& a,
        alia_kernel_timer_request const& b) const
    {
        // min-heap by fire time
        return a.fire_time > b.fire_time;
    }
};

// `alia_kernel` is the persistent object for one Alia component system.
struct alia_kernel
{
    // active transition and flare animations
    alia::animation_system animation;

    // pool table for this component tree
    alia_memory_system memory;

    // persistent substrate graph
    alia_substrate_system substrate;

    // current time in nanoseconds - The origin is arbitrary and the counter
    // may wrap.
    alia_nanosecond_count tick_count = 0;

    // refresh pass counter - This increments once per refresh. Passes that
    // belong to one incomplete refresh share a value.
    uint32_t refresh_counter = 0;

    // pool IDs registered with `memory`
    alia_memory_pool_id stack_pool = 0;
    alia_memory_pool_id scratch_pool = 0;
    alia_memory_pool_id substrate_discovery_pool = 0;
    alia_memory_pool_id substrate_storage_pool = 0;

    // pending timer requests, ordered by fire time
    std::priority_queue<
        alia_kernel_timer_request,
        std::vector<alia_kernel_timer_request>,
        alia_kernel_timer_request_compare>
        timer_requests;
    // timer event cycle counter - This prevents timer requests from being
    // serviced in the same frame that they're requested (which could throw
    // the event handler into a loop).
    uint64_t timer_event_cycle = 0;
    // timer dispatch cycle for the current update frame
    uint64_t update_timer_cycle = 0;

    // set when component state changes and cleared when a refresh completes
    bool dirty = true;
};

namespace alia {

// Set up resources for an already constructed kernel.
void
init_kernel(alia_kernel& kernel, alia_memory_ops memory_ops);

// Release resources owned by `kernel`. The object stays alive.
void
release_kernel(alia_kernel& kernel);

} // namespace alia
