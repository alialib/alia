#include <alia/context.hpp>
#include <alia/drawing.hpp>

namespace alia {

void
draw_box(
    display_list_arena& arena, box_command_list& list, box box, color color)
{
    box_draw_command* command = arena_alloc<box_draw_command>(arena);
    *command = box_draw_command{.box = box, .color = color, .next = nullptr};
    add_command(list, command);
}

} // namespace alia
