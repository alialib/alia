#ifndef ALIA_SYSTEM_INTERFACE_HPP
#define ALIA_SYSTEM_INTERFACE_HPP

#include <functional>

namespace alia {

struct system;

bool
system_needs_refresh(system const& sys);

void
refresh_system(system& sys);

void
set_error_handler(
    system& sys, std::function<void(std::exception_ptr)> handler);

} // namespace alia

#endif
