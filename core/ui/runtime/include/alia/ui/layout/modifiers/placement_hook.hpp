#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct context;

using placement_hook_node = layout_container;

// TODO: Give this a better name and maybe move it somewhere more generic.
struct placement_info
{
    alia::box box;
    float baseline;
};

struct placement_hook_scope
{
    layout_container_scope container;
    placement_info placement;
};

void
begin_placement_hook(
    context& ctx, placement_hook_scope& scope, layout_flag_set flags);

void
end_placement_hook(context& ctx, placement_hook_scope& scope);

template<class Content>
void
placement_hook(context& ctx, Content&& content)
{
    placement_hook_scope scope;
    begin_placement_hook(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)(scope.placement);
    end_placement_hook(ctx, scope);
}

template<class Content>
void
placement_hook(context& ctx, layout_flag_set flags, Content&& content)
{
    placement_hook_scope scope;
    begin_placement_hook(ctx, scope, flags);
    std::forward<Content>(content)(scope.placement);
    end_placement_hook(ctx, scope);
}

extern layout_node_vtable placement_hook_vtable;

} // namespace alia
