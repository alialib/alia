#pragma once

#include <alia/color.hpp>
#include <alia/geometry.hpp>
#include <alia/infinite_arena.hpp>

namespace alia {

struct box_draw_command
{
    box box;
    color color;
    box_draw_command* next;
};

template<class Command>
struct command_list
{
    Command* head;
    Command** tail_ptr;
    std::size_t count;
};

template<class Command>
void
clear_command_list(command_list<Command>& list)
{
    list.head = nullptr;
    list.tail_ptr = &list.head;
    list.count = 0;
}

template<class Command>
void
add_command(command_list<Command>& list, Command* command)
{
    *list.tail_ptr = command;
    list.tail_ptr = &command->next;
    ++list.count;
}

using display_list_arena = infinite_arena;

} // namespace alia
