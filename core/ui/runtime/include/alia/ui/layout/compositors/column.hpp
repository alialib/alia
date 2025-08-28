#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct context;

using column_layout_node = layout_container;

void
begin_column(
    context& ctx, layout_container_scope& scope, layout_flag_set flags);

void
end_column(context& ctx, layout_container_scope& scope);

template<class Content>
void
column(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_column(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

template<class Content>
void
column(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_column(ctx, scope, flags);
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

extern layout_node_vtable column_vtable;

} // namespace alia
