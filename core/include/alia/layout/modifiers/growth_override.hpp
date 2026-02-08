#pragma once

#include <utility>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

namespace alia {

struct growth_override_node
{
    layout_container container;
    float growth;
};

void
begin_growth_override(
    context& ctx, layout_container_scope& scope, float growth);

void
end_growth_override(context& ctx, layout_container_scope& scope);

extern alia_layout_node_vtable growth_override_vtable;

} // namespace alia
