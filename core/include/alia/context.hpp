#pragma once

#include <cstdint>

#include <alia/abi/events.h>

#include <alia/abi/context.h>
#include <alia/drawing.hpp>
#include <alia/layout/node.hpp>

namespace alia {

using context = alia_context;
using ui_system = alia_ui_system;
using ephemeral_context = alia_ephemeral_context;
using event_traversal = alia_event_traversal;

inline ui_system&
get_system(ephemeral_context& ctx)
{
    return *ctx.system;
}

// TODO: Figure out actual context interface protocols.
inline event_traversal&
get_event_traversal(ephemeral_context& ctx)
{
    return *ctx.event;
}

// TODO: Move elsewhere.
inline float
get_padding_size(ephemeral_context& ctx)
{
    return ctx.style->padding;
}

} // namespace alia
