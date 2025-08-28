#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

using flow_layout_node = layout_container;

extern layout_node_vtable flow_vtable;

void
begin_flow(context& ctx, layout_container_scope& scope, layout_flag_set flags);

void
end_flow(context& ctx, layout_container_scope& scope);

template<class Content>
void
flow(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_flow(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

template<class Content>
void
flow(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_flow(ctx, scope, flags);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

} // namespace alia
