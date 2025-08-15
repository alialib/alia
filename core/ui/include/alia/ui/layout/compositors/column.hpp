#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct Context;

using ColumnLayoutNode = LayoutContainer;

void
begin_column(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_column(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
column(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_column(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

template<class Content>
void
column(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_column(ctx, scope, flags);
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

extern LayoutNodeVtable column_vtable;

} // namespace alia
