#include <alia/ui/context.hpp>
#include <alia/ui/drawing.hpp>

namespace alia {

void
draw_box(DisplayListArena& arena, BoxCommandList& list, Box box, Color color)
{
    BoxDrawCommand* command = reinterpret_cast<BoxDrawCommand*>(
        arena.allocate(sizeof(BoxDrawCommand), alignof(BoxDrawCommand)));
    *command = BoxDrawCommand{.box = box, .color = color, .next = nullptr};
    add_command(list, command);
}

} // namespace alia
