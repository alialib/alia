#pragma once

#include <utility>

#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/compositors/column.hpp>
#include <alia/layout/container.hpp>

#include <alia/base/arena.h>

namespace alia {

struct grid_row_layout_node;

struct grid_scratch;

struct grid_layout_node
{
    alia_layout_node base = {};
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
begin_grid(context& ctx, grid_scope& scope, alia_layout_flags_t flags);

void
end_grid(context& ctx, grid_scope& scope);

void
begin_grid_row(
    context& ctx,
    grid_handle grid,
    layout_container_scope& scope,
    alia_layout_flags_t flags);

void
end_grid_row(context& ctx, layout_container_scope& scope);

} // namespace alia
