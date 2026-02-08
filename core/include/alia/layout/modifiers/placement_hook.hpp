#pragma once

#include <utility>

#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

// TODO: Use forward declarations once those are sorted out.
#include <alia/context.hpp>

namespace alia {

using placement_hook_node = layout_container;

// TODO: Give this a better name and maybe move it somewhere more generic.
struct placement_info
{
    alia_box box;
    float baseline;
};

struct placement_hook_scope
{
    layout_container_scope container;
    placement_info placement;
};

void
begin_placement_hook(
    context& ctx, placement_hook_scope& scope, alia_layout_flags_t flags);

void
end_placement_hook(context& ctx, placement_hook_scope& scope);

extern alia_layout_node_vtable placement_hook_vtable;

} // namespace alia
