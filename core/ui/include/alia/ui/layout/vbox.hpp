#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct Context;

using VBoxLayoutNode = LayoutContainer;

void
begin_vbox(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_vbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
vbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_vbox(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

template<class Content>
void
vbox(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_vbox(ctx, scope, flags);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

extern LayoutNodeVtable vbox_vtable;

} // namespace alia
