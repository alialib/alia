#include <alia/core/timing/scheduler.hpp>

namespace alia {

void
schedule_callback(
    timer_event_scheduler& scheduler,
    std::function<void()> callback,
    millisecond_count time)
{
    timer_event_request rq;
    rq.callback = std::move(callback);
    rq.trigger_time = time;
    rq.frame_issued = scheduler.frame_counter;
    scheduler.requests.push_back(rq);
}

void
issue_ready_events(
    timer_event_scheduler& scheduler,
    millisecond_count now,
    function_view<void(std::function<void()> const&)> issue)
{
    ++scheduler.frame_counter;
    while (true)
    {
        // Ideally, the list would be stored sorted, but it has to be
        // sorted relative to the current tick count (to handle wrapping),
        // and the list is generally not very long anyway.
        auto next_event = scheduler.requests.end();
        for (auto i = scheduler.requests.begin();
             i != scheduler.requests.end();
             ++i)
        {
            if (i->frame_issued != scheduler.frame_counter
                && int(now - i->trigger_time) >= 0
                && (next_event == scheduler.requests.end()
                    || int(next_event->trigger_time - i->trigger_time) >= 0))
            {
                next_event = i;
            }
        }
        if (next_event == scheduler.requests.end())
            break;

        timer_event_request request = *next_event;
        scheduler.requests.erase(next_event);

        issue(request.callback);
    }
}

bool
has_scheduled_events(timer_event_scheduler const& scheduler)
{
    return !scheduler.requests.empty();
}

millisecond_count
get_time_until_next_event(
    timer_event_scheduler& scheduler, millisecond_count now)
{
    auto next_event = scheduler.requests.end();
    for (auto i = scheduler.requests.begin(); i != scheduler.requests.end();
         ++i)
    {
        if (next_event == scheduler.requests.end()
            || int(next_event->trigger_time - i->trigger_time) >= 0)
        {
            next_event = i;
        }
    }
    return int(next_event->trigger_time - now) >= 0
               ? (next_event->trigger_time - now)
               : 0;
}

} // namespace alia
