#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct Context;

using HBoxLayoutNode = LayoutContainer;

void
begin_hbox(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_hbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
hbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_hbox(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

template<class Content>
void
hbox(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_hbox(ctx, scope, flags);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

extern LayoutNodeVtable hbox_vtable;

} // namespace alia
