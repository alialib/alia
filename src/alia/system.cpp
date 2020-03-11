#include <alia/system.hpp>

#include <chrono>

#include <alia/flow/events.hpp>

namespace alia {

millisecond_count
get_default_tick_count()
{
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<
               std::chrono::duration<millisecond_count, std::milli>>(
               now - start)
        .count();
}

void
refresh_system(system& sys)
{
    sys.refresh_needed = false;

    refresh_event refresh;
    impl::dispatch_event(sys, refresh);
}

} // namespace alia
