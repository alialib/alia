#pragma once

#include <utility>

#include <alia/geometry.hpp>
#include <alia/layout/container.hpp>
#include <alia/layout/flags.hpp>

namespace alia {

struct alignment_override_node
{
    layout_container container;
    layout_flag_set flags;
};

void
begin_alignment_override(
    context& ctx, layout_container_scope& scope, layout_flag_set flags);

void
end_alignment_override(context& ctx, layout_container_scope& scope);

template<class Content>
void
alignment_override(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_alignment_override(ctx, scope, flags);
    std::forward<Content>(content)();
    end_alignment_override(ctx, scope);
}

extern layout_node_vtable alignment_override_vtable;

} // namespace alia
