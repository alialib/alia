#pragma once

#include <utility>

#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

namespace alia {

using hyperflow_layout_node = layout_container;

extern alia_layout_node_vtable hyperflow_vtable;

void
begin_hyperflow(
    context& ctx, layout_container_scope& scope, alia_layout_flags_t flags);

void
end_hyperflow(context& ctx, layout_container_scope& scope);

} // namespace alia
