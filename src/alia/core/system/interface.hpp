#ifndef ALIA_CORE_SYSTEM_INTERFACE_HPP
#define ALIA_CORE_SYSTEM_INTERFACE_HPP

#include <exception>
#include <functional>

namespace alia {

struct untyped_system;

bool
system_needs_refresh(untyped_system& sys);

void
refresh_system(untyped_system& sys);

void
set_error_handler(
    untyped_system& sys, std::function<void(std::exception_ptr)> handler);

// Schedule an update to be applied to the system. This is intended to be
// called asynchronously, and in cases where the system is operating in a
// multi-threaded fashion, it should be safe to call from other threads. It
// will schedule the given function to run inside the UI thread (followed by a
// refresh).
void
schedule_asynchronous_update(
    untyped_system& sys, std::function<void()> update);

} // namespace alia

#endif
