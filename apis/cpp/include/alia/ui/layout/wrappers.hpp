#pragma once

#include <alia/abi/ui/layout/wrappers.h>
#include <alia/context.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

template<class Content>
void
alignment_override(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_alignment_override(ctx, scope, raw_code(flags));
    std::forward<Content>(content)();
    end_alignment_override(ctx, scope);
}

template<class Content>
void
placement_hook(context& ctx, Content&& content)
{
    placement_hook_scope scope;
    begin_placement_hook(ctx, scope, 0);
    std::forward<Content>(content)(scope.placement);
    end_placement_hook(ctx, scope);
}

template<class Content>
void
placement_hook(context& ctx, layout_flag_set flags, Content&& content)
{
    placement_hook_scope scope;
    begin_placement_hook(ctx, scope, raw_code(flags));
    std::forward<Content>(content)(scope.placement);
    end_placement_hook(ctx, scope);
}

template<class Content>
void
inset(context& ctx, alia_insets insets, Content&& content)
{
    layout_container_scope scope;
    begin_inset(ctx, scope, insets, 0);
    std::forward<Content>(content)();
    end_inset(ctx, scope);
}

template<class Content>
void
min_size_constraint(context& ctx, alia_vec2f min_size, Content&& content)
{
    layout_container_scope scope;
    begin_min_size_constraint(ctx, scope, min_size);
    std::forward<Content>(content)();
    end_min_size_constraint(ctx, scope);
}

template<class Content>
void
growth_override(context& ctx, float growth, Content&& content)
{
    layout_container_scope scope;
    begin_growth_override(ctx, scope, growth);
    std::forward<Content>(content)();
    end_growth_override(ctx, scope);
}

} // namespace alia
