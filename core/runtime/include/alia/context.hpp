#pragma once

#include <cstdint>

#include <alia/arenas.hpp>
#include <alia/display_list.hpp>
#include <alia/drawing.hpp>
#include <alia/event.h>
#include <alia/layout/node.hpp>

namespace alia {

struct ui_system;
struct layout_container;
struct event_traversal;

enum class pass_type
{
    Refresh,
    Draw,
    Event,
};

struct layout_emission
{
    alia_scratch_arena* arena;
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
    alia_draw_state* state;
};

struct event_pass
{
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
    ui_system* system;
    event_traversal* event;
};

using ephemeral_context = context;

// TODO: Figure out actual context interface protocols.
inline event_traversal&
get_event_traversal(ephemeral_context ctx)
{
    return *ctx.event;
}

} // namespace alia
