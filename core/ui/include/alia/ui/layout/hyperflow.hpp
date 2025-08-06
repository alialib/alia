#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

using HyperflowLayoutNode = LayoutContainer;

extern LayoutNodeVtable hyperflow_vtable;

void
begin_hyperflow(
    Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_hyperflow(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
hyperflow(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_hyperflow(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_hyperflow(ctx, scope);
}

template<class Content>
void
hyperflow(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_hyperflow(ctx, scope, flags);
    std::forward<Content>(content)();
    end_hyperflow(ctx, scope);
}

} // namespace alia
