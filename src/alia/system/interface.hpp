#ifndef ALIA_SYSTEM_INTERFACE_HPP
#define ALIA_SYSTEM_INTERFACE_HPP

#include <functional>
#include <exception>

namespace alia {

struct system;

bool
system_needs_refresh(system const& sys);

void
refresh_system(system& sys);

void
set_error_handler(
    system& sys, std::function<void(std::exception_ptr)> handler);

// Schedule an update to be applied to the system. This is intended to be
// called asynchronously, and in cases where the system is operating in a
// multi-threaded fashion, it should be safe to call from other threads. It
// will schedule the given function to run inside the UI thread (followed by a
// refresh).
void
schedule_asynchronous_update(system& sys, std::function<void()> update);

} // namespace alia

#endif
