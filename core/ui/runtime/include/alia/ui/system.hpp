#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/system.hpp>

namespace alia {

struct system
{
    layout_system layout;

    vec2 framebuffer_size;
    vec2 ui_zoom;
};

void
initialize(system& system, vec2 framebuffer_size, vec2 ui_zoom);

} // namespace alia
