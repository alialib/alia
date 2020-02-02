#include <alia/components/system.hpp>

#include <chrono>

#include <alia/flow/events.hpp>

namespace alia {

namespace {

millisecond_count
get_tick_count_now()
{
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<
               std::chrono::duration<millisecond_count, std::milli>>(
               now - start)
        .count();
}

} // namespace

void
refresh_system(system& sys)
{
    if (sys.automatic_time_updates)
        sys.tick_counter = get_tick_count_now();

    sys.refresh_needed = false;

    refresh_event refresh;
    dispatch_event(sys, refresh);
}

} // namespace alia
