#pragma once

#include <utility>

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct GrowthOverrideNode
{
    LayoutContainer container;
    float growth;
};

void
begin_growth_override(Context& ctx, LayoutContainerScope& scope, float growth);

void
end_growth_override(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
growth_override(Context& ctx, float growth, Content&& content)
{
    LayoutContainerScope scope;
    begin_growth_override(ctx, scope, growth);
    std::forward<Content>(content)();
    end_growth_override(ctx, scope);
}

extern LayoutNodeVtable growth_override_vtable;

} // namespace alia
