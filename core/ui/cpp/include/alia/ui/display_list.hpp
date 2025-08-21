#pragma once

#include <alia/kernel/infinite_arena.hpp>
#include <alia/ui/color.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

struct BoxDrawCommand
{
    Box box;
    Color color;
    BoxDrawCommand* next;
};

template<class Command>
struct CommandList
{
    Command* head;
    Command** tail_ptr;
    size_t count;
};

template<class Command>
void
clear_command_list(CommandList<Command>& list)
{
    list.head = nullptr;
    list.tail_ptr = &list.head;
    list.count = 0;
}

template<class Command>
void
add_command(CommandList<Command>& list, Command* command)
{
    *list.tail_ptr = command;
    list.tail_ptr = &command->next;
    ++list.count;
}

using DisplayListArena = InfiniteArena;

} // namespace alia
