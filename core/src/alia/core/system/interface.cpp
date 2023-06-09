#include <alia/core/system/interface.hpp>

#include <chrono>

#include <alia/core/system/internals.hpp>

namespace alia {

bool
system_needs_refresh(system const& sys)
{
    return sys.refresh_needed;
}

void
refresh_system(system& sys)
{
    sys.refresh_needed = false;
    ++sys.refresh_counter;

    // TODO: Track pass_count and handle excessive counts in useful ways.
    // ("Useful" is probably different for development/release builds.)
    // int pass_count = 0;
    while (true)
    {
        refresh_event refresh;
        detail::dispatch_event(sys, refresh);
        if (!sys.root_component->dirty)
            break;
        // ++pass_count;
        // assert(pass_count < 64);
    };
}

void
set_error_handler(system& sys, std::function<void(std::exception_ptr)> handler)
{
    sys.error_handler = handler;
}

void
schedule_asynchronous_update(system& sys, std::function<void()> update)
{
    sys.external->schedule_asynchronous_update(std::move(update));
}

} // namespace alia
