#pragma once

#include <alia/ui/display_list.hpp>

namespace alia {

// TODO: Take a context parameter.
void
draw_box(DisplayList* display_list, Box box, Color color);

} // namespace alia
