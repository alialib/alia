#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

#include <alia/abi/prelude.h>

namespace alia {

// Shared due-selection logic between production timer dispatch and unit
// tests. The "emit" callback is injected so tests can observe which timers
// would be dispatched without relying on refresh/dispatch side effects.
template<class TimerPQ, class EmitFn>
void
process_due_timers_internal(
    TimerPQ& pq, alia_nanosecond_count now, uint64_t cycle, EmitFn&& emit)
{
    using request_t
        = std::remove_cv_t<std::remove_reference_t<decltype(pq.top())>>;

    std::vector<request_t> deferred;

    while (!pq.empty())
    {
        auto const& top = pq.top();

        // Wrap-safe "is due?" check: dispatch when now - fire_time >= 0.
        if (static_cast<int64_t>(now - top.fire_time) < 0)
            break;

        request_t req = top;
        pq.pop();

        // Defer timers queued during this same dispatch cycle to avoid
        // re-entrancy loops.
        if (req.queued_in_cycle == cycle)
        {
            deferred.push_back(req);
            continue;
        }

        emit(req);
    }

    for (auto const& req : deferred)
        pq.push(req);
}

} // namespace alia
