#pragma once

#include <cstdint>

#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/protocol.h>

// TODO: Use forward declarations once those are sorted out.
#include <alia/context.hpp>

namespace alia {

struct layout_container
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    alia_layout_node* first_child;
};

struct layout_container_scope
{
    layout_container* container;
};

void
begin_container(
    context& ctx,
    layout_container_scope& scope,
    alia_layout_node_vtable* vtable,
    alia_layout_flags_t flags);

void
end_container(context& ctx, layout_container_scope& scope);

} // namespace alia
