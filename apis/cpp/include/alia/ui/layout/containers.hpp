#pragma once

#include <alia/abi/ui/layout/containers.h>
#include <alia/context.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

template<class Content>
void
row(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_row(ctx, scope, 0);
    std::forward<Content>(content)();
    end_row(ctx, scope);
}

template<class Content>
void
row(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_row(ctx, scope, raw_code(flags));
    std::forward<Content>(content)();
    end_row(ctx, scope);
}

template<class Content>
void
column(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_column(ctx, scope, 0);
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

template<class Content>
void
column(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_column(ctx, scope, raw_code(flags));
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

template<class Content>
void
flow(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_flow(ctx, scope, 0);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

template<class Content>
void
flow(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_flow(ctx, scope, raw_code(flags));
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

template<class Content>
void
hyperflow(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_hyperflow(ctx, scope, 0);
    std::forward<Content>(content)();
    end_hyperflow(ctx, scope);
}

template<class Content>
void
hyperflow(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_hyperflow(ctx, scope, raw_code(flags));
    std::forward<Content>(content)();
    end_hyperflow(ctx, scope);
}

template<class Content>
void
grid(context& ctx, Content&& content)
{
    grid_scope scope;
    begin_grid(ctx, scope, 0);
    std::forward<Content>(content)(&scope);
    end_grid(ctx, scope);
}

template<class Content>
void
grid(context& ctx, layout_flag_set flags, Content&& content)
{
    grid_scope scope;
    begin_grid(ctx, scope, raw_code(flags));
    std::forward<Content>(content)(&scope);
    end_grid(ctx, scope);
}

template<class Content>
void
grid_row(context& ctx, grid_handle grid, Content&& content)
{
    layout_container_scope scope;
    begin_grid_row(ctx, grid, scope, 0);
    std::forward<Content>(content)();
    end_grid_row(ctx, scope);
}

template<class Content>
void
grid_row(
    context& ctx, grid_handle grid, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_grid_row(ctx, grid, scope, raw_code(flags));
    std::forward<Content>(content)();
    end_grid_row(ctx, scope);
}

} // namespace alia
