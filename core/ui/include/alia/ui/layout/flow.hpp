#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

using FlowLayoutNode = LayoutContainer;

extern LayoutNodeVtable flow_vtable;

void
begin_flow(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_flow(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
flow(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_flow(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

template<class Content>
void
flow(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_flow(ctx, scope, flags);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

} // namespace alia
