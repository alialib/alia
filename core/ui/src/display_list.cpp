#include <alia/ui/display_list.hpp>

namespace alia {

void
init_display_list(DisplayList& list)
{
    list.arena.initialize();
    clear_command_list(list.boxes);
}

void
reset_display_list(DisplayList& list)
{
    list.arena.reset();
    clear_command_list(list.boxes);
}

void
destroy_display_list(DisplayList& list)
{
    list.arena.release();
}

} // namespace alia
