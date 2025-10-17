#pragma once

#include <alia/display_list.hpp>

namespace alia {

using box_command_list = command_list<box_draw_command>;

// TODO: Take a context parameter.
void
draw_box(
    display_list_arena& arena, box_command_list& list, box box, color color);

} // namespace alia
