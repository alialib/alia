#include <alia/ui/display_list.hpp>

#include <alia/foundation/arena.hpp>

namespace alia {

DisplayList
create_display_list(Arena* arena)
{
    // TODO: Handle dynamic allocation of commands.
    DrawCommand* commands = (DrawCommand*) arena_alloc(
        arena, sizeof(DrawCommand) * 4096); // TODO
    return DisplayList{arena, commands, 0};
}

void
reset_display_list(DisplayList* display_list)
{
    reset_arena(display_list->arena);
    // TODO: Handle dynamic allocation of commands.
    display_list->commands = (DrawCommand*) arena_alloc(
        display_list->arena, sizeof(DrawCommand) * 4096); // TODO
    display_list->count = 0;
}

void
destroy_display_list(DisplayList* display_list)
{
    destroy_arena(display_list->arena);
}

} // namespace alia
