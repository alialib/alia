#pragma once

#include <utility>

#include <alia/geometry.hpp>
#include <alia/layout/container.hpp>
#include <alia/layout/flags.hpp>

namespace alia {

struct inset_layout_node
{
    layout_container container;
    alia_insets insets;
};

void
begin_inset(
    context& ctx,
    layout_container_scope& scope,
    alia_insets insets,
    layout_flag_set flags);

void
end_inset(context& ctx, layout_container_scope& scope);

template<class Content>
void
inset(context& ctx, alia_insets insets, Content&& content)
{
    layout_container_scope scope;
    begin_inset(ctx, scope, insets, NO_FLAGS);
    std::forward<Content>(content)();
    end_inset(ctx, scope);
}

extern layout_node_vtable inset_vtable;

} // namespace alia
