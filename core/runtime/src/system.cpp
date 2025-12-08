#include <alia/system/object.hpp>

namespace alia {

void
initialize(ui_system& system, vec2 surface_size)
{
    initialize(system.layout);
    system.surface_size = surface_size;
}

} // namespace alia
