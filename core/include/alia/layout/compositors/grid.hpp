#pragma once

#include <utility>

#include <alia/layout/compositors/column.hpp>
#include <alia/layout/container.hpp>
#include <alia/layout/flags.hpp>

#include <alia/internals/arena.hpp>

namespace alia {

struct grid_row_layout_node;

struct grid_scratch;

struct grid_layout_node
{
    layout_node base = {};
    grid_row_layout_node* first_row = nullptr;
    grid_scratch* scratch = nullptr;
    column_layout_node column = {};
    alia_arena_marker scratch_marker = {};
};

struct grid_row_layout_node
{
    layout_container container = {};
    grid_layout_node* grid = nullptr;
    alia_arena_marker scratch_marker = {};
    grid_row_layout_node* next_row = nullptr;
};

struct grid_scope
{
    grid_layout_node* grid = nullptr;
    grid_row_layout_node** next_row_ptr = nullptr;
};

using grid_handle = grid_scope*;

void
begin_grid(context& ctx, grid_scope& scope, layout_flag_set flags);

void
end_grid(context& ctx, grid_scope& scope);

template<class Content>
void
grid(context& ctx, Content&& content)
{
    grid_scope scope;
    begin_grid(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)(&scope);
    end_grid(ctx, scope);
}

template<class Content>
void
grid(context& ctx, layout_flag_set flags, Content&& content)
{
    grid_scope scope;
    begin_grid(ctx, scope, flags);
    std::forward<Content>(content)(&scope);
    end_grid(ctx, scope);
}

void
begin_grid_row(
    context& ctx,
    grid_handle grid,
    layout_container_scope& scope,
    layout_flag_set flags);

void
end_grid_row(context& ctx, layout_container_scope& scope);

template<class Content>
void
grid_row(context& ctx, grid_handle grid, Content&& content)
{
    layout_container_scope scope;
    begin_grid_row(ctx, grid, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_grid_row(ctx, scope);
}

template<class Content>
void
grid_row(
    context& ctx, grid_handle grid, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_grid_row(ctx, grid, scope, flags);
    std::forward<Content>(content)();
    end_grid_row(ctx, scope);
}

} // namespace alia
