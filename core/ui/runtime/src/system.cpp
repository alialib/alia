#include <alia/ui/system.hpp>

namespace alia {

void
initialize(system& system, vec2 framebuffer_size, vec2 ui_zoom)
{
    initialize(system.layout);
    system.framebuffer_size = framebuffer_size;
    system.ui_zoom = ui_zoom;
}

} // namespace alia
