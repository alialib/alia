#pragma once

#include <utility>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

namespace alia {

struct alignment_override_node
{
    layout_container container;
    alia_layout_flags_t flags;
};

void
begin_alignment_override(
    context& ctx, layout_container_scope& scope, alia_layout_flags_t flags);

void
end_alignment_override(context& ctx, layout_container_scope& scope);

extern alia_layout_node_vtable alignment_override_vtable;

} // namespace alia
