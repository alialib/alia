#include <alia/ui/context.hpp>
#include <alia/ui/drawing.hpp>

namespace alia {

void
draw_box(DisplayList* display_list, Box box, Color color)
{
    BoxDrawCommand* command
        = reinterpret_cast<BoxDrawCommand*>(display_list->arena.allocate(
            sizeof(BoxDrawCommand), alignof(BoxDrawCommand)));
    *command = BoxDrawCommand{.box = box, .color = color, .next = nullptr};
    add_command(display_list->boxes, command);
}

} // namespace alia
