#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/system.hpp>

namespace alia {

struct System
{
    LayoutSystem layout;

    Vec2 framebuffer_size;
    Vec2 ui_zoom;
};

void
initialize(System& system, Vec2 framebuffer_size, Vec2 ui_zoom);

} // namespace alia
