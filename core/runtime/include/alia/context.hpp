#pragma once

#include <cstdint>

#include <alia/arenas.hpp>
#include <alia/display_list.hpp>
#include <alia/drawing.hpp>
#include <alia/events.h>
#include <alia/layout/node.hpp>

namespace alia {

struct ui_system;
struct layout_container;
struct event_traversal;

struct style
{
    float padding;
};

struct context
{
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
