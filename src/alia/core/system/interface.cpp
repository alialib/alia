#include <alia/core/system/interface.hpp>

#include <alia/core/system/internals.hpp>

namespace alia {

bool
system_needs_refresh(untyped_system& sys)
{
    return sys.refresh_needed;
}

void
refresh_system(untyped_system& sys)
{
    sys.refresh_needed = false;
    ++sys.refresh_counter;

    // TODO: Track pass_count and handle excessive counts in useful ways.
    // ("Useful" is probably different for development/release builds.)
    // int pass_count = 0;
    while (true)
    {
        refresh_event refresh;
        detail::dispatch_untargeted_event(sys, refresh);
        if (!sys.root_component->dirty)
            break;
        // ++pass_count;
        // assert(pass_count < 64);
    };
}

void
set_error_handler(
    untyped_system& sys, std::function<void(std::exception_ptr)> handler)
{
    sys.error_handler = handler;
}

void
schedule_asynchronous_update(untyped_system& sys, std::function<void()> update)
{
    sys.external->schedule_asynchronous_update(std::move(update));
}

} // namespace alia
