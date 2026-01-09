#pragma once

#include <alia/display_list.hpp>

#include <alia/abi/arena.h>

namespace alia {

using box_command_list = command_list<box_draw_command>;

// TODO: Take a context parameter.
void
draw_box(
    display_list_arena& arena, box_command_list& list, box box, color color);

} // namespace alia

extern "C" struct alia_draw_state
{
    alia_arena_view* arena;
    alia::box_command_list* box_command_list;
};
