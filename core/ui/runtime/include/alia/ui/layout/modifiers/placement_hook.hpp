#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct Context;

using PlacementHookNode = LayoutContainer;

// TODO: Give this a better name and maybe move it somewhere more generic.
struct PlacementInfo
{
    Box box;
    float baseline;
};

struct PlacementHookScope
{
    LayoutContainerScope container;
    PlacementInfo placement;
};

void
begin_placement_hook(
    Context& ctx, PlacementHookScope& scope, LayoutFlagSet flags);

void
end_placement_hook(Context& ctx, PlacementHookScope& scope);

template<class Content>
void
placement_hook(Context& ctx, Content&& content)
{
    PlacementHookScope scope;
    begin_placement_hook(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)(scope.placement);
    end_placement_hook(ctx, scope);
}

template<class Content>
void
placement_hook(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    PlacementHookScope scope;
    begin_placement_hook(ctx, scope, flags);
    std::forward<Content>(content)(scope.placement);
    end_placement_hook(ctx, scope);
}

extern LayoutNodeVtable placement_hook_vtable;

} // namespace alia
