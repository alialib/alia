#pragma once

#include <utility>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

namespace alia {

struct min_size_node
{
    layout_container container;
    alia_vec2f min_size;
};

void
begin_min_size_constraint(
    context& ctx, layout_container_scope& scope, alia_vec2f min_size);

void
end_min_size_constraint(context& ctx, layout_container_scope& scope);

extern alia_layout_node_vtable min_size_vtable;

} // namespace alia
