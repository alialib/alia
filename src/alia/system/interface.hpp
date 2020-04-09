#ifndef ALIA_SYSTEM_INTERFACE_HPP
#define ALIA_SYSTEM_INTERFACE_HPP

namespace alia {

struct system;

bool
system_needs_refresh(system const& sys);

void
refresh_system(system& sys);

} // namespace alia

#endif
