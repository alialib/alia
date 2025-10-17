#pragma once

#include <utility>

#include <alia/layout/container.hpp>
#include <alia/layout/flags.hpp>

namespace alia {

using hyperflow_layout_node = layout_container;

extern layout_node_vtable hyperflow_vtable;

void
begin_hyperflow(
    context& ctx, layout_container_scope& scope, layout_flag_set flags);

void
end_hyperflow(context& ctx, layout_container_scope& scope);

template<class Content>
void
hyperflow(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_hyperflow(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_hyperflow(ctx, scope);
}

template<class Content>
void
hyperflow(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_hyperflow(ctx, scope, flags);
    std::forward<Content>(content)();
    end_hyperflow(ctx, scope);
}

} // namespace alia
