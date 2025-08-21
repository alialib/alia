#pragma once

#include <alia/ui/display_list.hpp>

namespace alia {

using BoxCommandList = CommandList<BoxDrawCommand>;

// TODO: Take a context parameter.
void
draw_box(DisplayListArena& arena, BoxCommandList& list, Box box, Color color);

} // namespace alia
