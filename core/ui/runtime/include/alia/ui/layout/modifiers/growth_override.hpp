#pragma once

#include <utility>

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct growth_override_node
{
    layout_container container;
    float growth;
};

void
begin_growth_override(
    context& ctx, layout_container_scope& scope, float growth);

void
end_growth_override(context& ctx, layout_container_scope& scope);

template<class Content>
void
growth_override(context& ctx, float growth, Content&& content)
{
    layout_container_scope scope;
    begin_growth_override(ctx, scope, growth);
    std::forward<Content>(content)();
    end_growth_override(ctx, scope);
}

extern layout_node_vtable growth_override_vtable;

} // namespace alia
