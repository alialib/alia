#pragma once

#include <utility>

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct min_size_node
{
    layout_container container;
    vec2 min_size;
};

void
begin_min_size_constraint(
    context& ctx, layout_container_scope& scope, vec2 min_size);

void
end_min_size_constraint(context& ctx, layout_container_scope& scope);

template<class Content>
void
min_size_constraint(context& ctx, vec2 min_size, Content&& content)
{
    layout_container_scope scope;
    begin_min_size_constraint(ctx, scope, min_size);
    std::forward<Content>(content)();
    end_min_size_constraint(ctx, scope);
}

extern layout_node_vtable min_size_vtable;

} // namespace alia
