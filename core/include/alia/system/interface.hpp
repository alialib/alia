#pragma once

#include <exception>
#include <functional>

// TODO: Use forward declarations once those are sorted out.
#include <alia/system/object.hpp>

namespace alia {

bool
system_needs_refresh(ui_system& sys);

void
refresh_system(ui_system& sys);

void
set_error_handler(
    ui_system& sys, std::function<void(std::exception_ptr)> handler);

// Schedule an update to be applied to the system. This is intended to be
// called asynchronously, and in cases where the system is operating in a
// multi-threaded fashion, it should be safe to call from other threads. It
// will schedule the given function to run inside the UI thread (followed by a
// refresh).
void
schedule_asynchronous_update(ui_system& sys, std::function<void()> update);

} // namespace alia
