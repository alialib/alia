#pragma once

#include <cstdint>

#include <alia/display_list.hpp>
#include <alia/drawing.hpp>

namespace alia {

struct event;
struct system;
struct layout_node;
struct layout_container;
struct infinite_arena;

enum class pass_type
{
    Refresh,
    Draw,
    Event,
};

struct layout_emission
{
    infinite_arena* arena;
    layout_node** next_ptr;
};

struct style
{
    float padding;
};

struct refresh_pass
{
    layout_emission layout_emission;
};

struct draw_pass
{
    display_list_arena* display_list_arena;
    box_command_list* box_command_list;
};

struct event_pass
{
    alia::event* event;
};

struct pass
{
    pass_type type;
    union
    {
        refresh_pass refresh;
        draw_pass draw;
        event_pass event;
    };
};

struct context
{
    pass pass;
    style* style;
    system* system;
};

using ephemeral_context = context;

} // namespace alia
