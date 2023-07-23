#ifndef ALIA_CORE_SYSTEM_INTERFACE_HPP
#define ALIA_CORE_SYSTEM_INTERFACE_HPP

#include <chrono>
#include <exception>
#include <functional>

#include <alia/core/flow/top_level.hpp>
#include <alia/core/system/internals.hpp>

namespace alia {

template<class System>
bool
system_needs_refresh(System const& sys)
{
    return sys.refresh_needed;
}

template<class System>
void
set_error_handler(System& sys, std::function<void(std::exception_ptr)> handler)
{
    sys.error_handler = handler;
}

// Schedule an update to be applied to the system. This is intended to be
// called asynchronously, and in cases where the system is operating in a
// multi-threaded fashion, it should be safe to call from other threads. It
// will schedule the given function to run inside the UI thread (followed by a
// refresh).
template<class System>
void
schedule_asynchronous_update(System& sys, std::function<void()> update)
{
    sys.external->schedule_asynchronous_update(std::move(update));
}

} // namespace alia

#endif
