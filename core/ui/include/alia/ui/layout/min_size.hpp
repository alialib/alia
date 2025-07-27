#pragma once

#include <utility>

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct MinSizeNode
{
    LayoutContainer container;
    Vec2 min_size;
};

void
begin_min_size(Context& ctx, LayoutContainerScope& scope, Vec2 min_size);

void
end_min_size(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
min_size(Context& ctx, Vec2 min_size, Content&& content)
{
    LayoutContainerScope scope;
    begin_min_size(ctx, scope, min_size);
    std::forward<Content>(content)();
    end_min_size(ctx, scope);
}

extern LayoutNodeVtable min_size_vtable;

} // namespace alia
