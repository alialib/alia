#include <alia/ui/context.hpp>
#include <alia/ui/drawing.hpp>

namespace alia {

void
draw_box(DisplayList* display_list, Box box, Color color)
{
    // TODO: Allocate a new command via the arena.
    display_list->commands[display_list->count++]
        = {.type = DrawCommandType::Box, .box = box, .color = color};
}

} // namespace alia
