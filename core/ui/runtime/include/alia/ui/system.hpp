#pragma once

#include <alia/kernel/animation/flares.hpp>
#include <alia/kernel/animation/transitions.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/system.hpp>

namespace alia {

struct animation_tracking_system
{
    // active transitions
    transition_animation_map transitions;
    // active flare groups
    flare_map flares;
};

struct system
{
    layout_system layout;
    animation_tracking_system animation;
    vec2 framebuffer_size;
    vec2 ui_zoom;
};

void
initialize(system& system, vec2 framebuffer_size, vec2 ui_zoom);

} // namespace alia
