#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct Context;

using RowLayoutNode = LayoutContainer;

void
begin_row(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_row(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
row(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_row(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_row(ctx, scope);
}

template<class Content>
void
row(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_row(ctx, scope, flags);
    std::forward<Content>(content)();
    end_row(ctx, scope);
}

extern LayoutNodeVtable row_vtable;

} // namespace alia
