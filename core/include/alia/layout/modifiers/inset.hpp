#pragma once

#include <utility>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

namespace alia {

struct inset_layout_node
{
    layout_container container;
    alia_insets insets;
};

void
begin_inset(
    context& ctx,
    layout_container_scope& scope,
    alia_insets insets,
    alia_layout_flags_t flags);

void
end_inset(context& ctx, layout_container_scope& scope);

extern alia_layout_node_vtable inset_vtable;

} // namespace alia
