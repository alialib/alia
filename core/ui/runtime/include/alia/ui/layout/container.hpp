#pragma once

#include <cstdint>

#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

struct context;
struct layout_node_vtable;

struct layout_container
{
    layout_node base;
    layout_flag_set flags;
    layout_node* first_child;
};

struct layout_container_scope
{
    layout_container* container;
};

void
begin_container(
    context& ctx,
    layout_container_scope& scope,
    layout_node_vtable* vtable,
    layout_flag_set flags);

void
end_container(context& ctx, layout_container_scope& scope);

} // namespace alia
