#include <alia/components/system.hpp>

#include <chrono>

#include <alia/flow/events.hpp>

namespace alia {

void
refresh_system(system& sys)
{
    refresh_event refresh;
    dispatch_event(sys, refresh);
}

} // namespace alia
