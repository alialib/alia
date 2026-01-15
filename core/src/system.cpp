#include <alia/system/object.hpp>

#include <alia/layout/system.hpp>

namespace alia {

void
initialize_ui_system(alia_ui_system* system, alia_vec2f surface_size)
{
    initialize(system->layout);
    system->surface_size = surface_size;
}

} // namespace alia
