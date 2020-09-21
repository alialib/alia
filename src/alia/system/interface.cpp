#include <alia/system/interface.hpp>

#include <chrono>

#include <alia/system/internals.hpp>

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

    int pass_count = 0;
    while (true)
    {
        refresh_event refresh;
        impl::dispatch_event(sys, refresh);
        if (!sys.root_component->dirty)
            break;
        ++pass_count;
        assert(pass_count < 64);
    };
}

void
set_error_handler(system& sys, std::function<void(std::exception_ptr)> handler)
{
    sys.error_handler = handler;
}

} // namespace alia
