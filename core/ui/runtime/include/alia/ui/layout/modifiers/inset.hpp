#pragma once

#include <utility>

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct InsetLayoutNode
{
    LayoutContainer container;
    Insets insets;
};

void
begin_inset(
    Context& ctx,
    LayoutContainerScope& scope,
    Insets insets,
    LayoutFlagSet flags);

void
end_inset(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
inset(Context& ctx, Insets insets, Content&& content)
{
    LayoutContainerScope scope;
    begin_inset(ctx, scope, insets, NO_FLAGS);
    std::forward<Content>(content)();
    end_inset(ctx, scope);
}

extern LayoutNodeVtable inset_vtable;

} // namespace alia
