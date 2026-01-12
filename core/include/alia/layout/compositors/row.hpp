#pragma once

#include <utility>

#include <alia/layout/container.hpp>
#include <alia/layout/flags.hpp>

// TODO: Use forward declarations once those are sorted out.
#include <alia/context.hpp>

namespace alia {

using row_layout_node = layout_container;

void
begin_row(context& ctx, layout_container_scope& scope, layout_flag_set flags);

void
end_row(context& ctx, layout_container_scope& scope);

template<class Content>
void
row(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_row(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_row(ctx, scope);
}

template<class Content>
void
row(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_row(ctx, scope, flags);
    std::forward<Content>(content)();
    end_row(ctx, scope);
}

extern layout_node_vtable row_vtable;

} // namespace alia
