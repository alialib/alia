#pragma once

#include <utility>

#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

// TODO: Use forward declarations once those are sorted out.
#include <alia/context.hpp>

namespace alia {

using row_layout_node = layout_container;

void
begin_row(
    context& ctx, layout_container_scope& scope, alia_layout_flags_t flags);

void
end_row(context& ctx, layout_container_scope& scope);

extern alia_layout_node_vtable row_vtable;

} // namespace alia
