#pragma once

// TODO: Sort this out.

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/drawing.h>
#include <alia/color.hpp>

namespace alia {

// TODO: Not this.
extern alia_draw_material_id box_material_id;

struct box_draw_command
{
    alia_draw_command base;
    alia_box box;
    alia_rgba color;
    float radius;
};

void
draw_box(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_rgba color,
    float radius);

} // namespace alia
