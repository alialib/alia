#include <alia/abi/ui/layout/components.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/context.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>
#include <alia/ui/layout/components/column.h>

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
    alia_layout_container container = {};
    grid_layout_node* grid = nullptr;
    alia_arena_marker scratch_marker = {};
    grid_row_layout_node* next_row = nullptr;
};

struct grid_scratch
{
    int column_count = 0;
    alia_horizontal_requirements* columns = nullptr;
    float total_width = 0, total_growth = 0;
};

struct grid_row_scratch
{
    float height = 0, ascent = 0;
};

int
count_columns(grid_layout_node* grid)
{
    // TODO: Add faster paths.
    int max_column_count = 0;
    for (auto row = grid->first_row; row; row = row->next_row)
    {
        int column_count = 0;
        for (auto child = row->container.first_child; child;
             child = child->next_sibling)
        {
            ++column_count;
        }
        max_column_count = (std::max) (max_column_count, column_count);
    }
    return max_column_count;
}

alia_horizontal_requirements
grid_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    grid.scratch = &claim_scratch<grid_scratch>(ctx->scratch);
    grid.scratch->column_count = count_columns(&grid);
    grid.scratch->columns = arena_alloc_array<alia_horizontal_requirements>(
        ctx->scratch, grid.scratch->column_count);
    // All the work of actually measuring the horizontal requirements of the
    // grid rows is done up-front, here. The grid rows will be contained within
    // the nested column below, and they will simply report these results back
    // to the column (uniformly).
    for (auto row = grid.first_row; row; row = row->next_row)
    {
        row->scratch_marker = alia_arena_mark(&ctx->scratch);
        claim_scratch<grid_row_scratch>(ctx->scratch);
        int column_index = 0;
        for (alia_layout_node* child = row->container.first_child;
             child != nullptr;
             child = child->next_sibling, ++column_index)
        {
            auto const child_x = alia_measure_horizontal(ctx, child);
            grid.scratch->columns[column_index].growth_factor
                = (std::max) (grid.scratch->columns[column_index]
                                  .growth_factor,
                              child_x.growth_factor);
            grid.scratch->columns[column_index].min_size
                = (std::max) (grid.scratch->columns[column_index].min_size,
                              child_x.min_size);
        }
    }
    float total_width = 0, total_growth = 0;
    for (int i = 0; i < grid.scratch->column_count; ++i)
    {
        total_width += grid.scratch->columns[i].min_size;
        total_growth += grid.scratch->columns[i].growth_factor;
    }
    grid.scratch->total_width = total_width;
    grid.scratch->total_growth = total_growth;
    // We still invoke the column, even though we know what the rows are going
    // to contribute to it. There might be other nodes in the column that
    // affect the results.
    grid.scratch_marker = alia_arena_mark(&ctx->scratch);
    return column_measure_horizontal(
        ctx, upcast<alia_layout_node>(&grid.column));
}

void
grid_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    alia_arena_jump(&ctx->scratch, grid.scratch_marker);
    column_assign_widths(
        ctx,
        main_axis,
        upcast<alia_layout_node>(&grid.column),
        assigned_width);
}

alia_vertical_requirements
grid_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    alia_arena_jump(&ctx->scratch, grid.scratch_marker);
    return column_measure_vertical(
        ctx,
        main_axis,
        upcast<alia_layout_node>(&grid.column),
        assigned_width);
}

void
grid_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    alia_arena_jump(&ctx->scratch, grid.scratch_marker);
    column_assign_boxes(
        ctx, main_axis, upcast<alia_layout_node>(&grid.column), box, baseline);
}

alia_layout_node_vtable grid_vtable
    = {grid_measure_horizontal,
       grid_assign_widths,
       grid_measure_vertical,
       grid_assign_boxes,
       grid_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

alia_horizontal_requirements
grid_row_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = alia_arena_mark(&ctx->scratch);
    alia_arena_jump(&ctx->scratch, grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(ctx->scratch);
    alia_arena_jump(&ctx->scratch, marker);
    return alia_horizontal_requirements{
        .min_size = grid_row.grid->scratch->total_width,
        .growth_factor = alia_resolve_growth_factor(grid_row.container.flags)};
}

void
grid_row_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = alia_arena_mark(&ctx->scratch);
    alia_arena_jump(&ctx->scratch, grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(ctx->scratch);
    // TODO: Implement.
    alia_arena_jump(&ctx->scratch, marker);
}

alia_vertical_requirements
grid_row_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = alia_arena_mark(&ctx->scratch);
    alia_arena_jump(&ctx->scratch, grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(ctx->scratch);
    auto& grid = *grid_row.grid;
    auto const placement = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(grid_row.container.flags, main_axis),
        assigned_width,
        grid.scratch->total_width);
    float const total_extra_space
        = (std::max) (0.f, placement.size - grid.scratch->total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, grid.scratch->total_growth);
    float height = 0, ascent = 0, descent = 0;
    auto const* column_data = grid.scratch->columns;
    for (alia_layout_node* child = grid_row.container.first_child;
         child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *column_data++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y = alia_measure_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, child_x.min_size + extra_space);
        height = (std::max) (height, child_y.min_size);
        ascent = (std::max) (ascent, child_y.ascent);
        descent = (std::max) (descent, child_y.descent);
    }
    scratch.height = height;
    scratch.ascent = ascent;
    alia_arena_jump(&ctx->scratch, marker);
    return alia_vertical_requirements{
        .min_size = (std::max) (height, ascent + descent),
        .growth_factor = alia_resolve_growth_factor(grid_row.container.flags),
        .ascent = ascent,
        .descent = descent};
}

void
grid_row_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = alia_arena_mark(&ctx->scratch);
    alia_arena_jump(&ctx->scratch, grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(ctx->scratch);
    auto& grid = *grid_row.grid;
    auto const placement = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(grid_row.container.flags, main_axis),
        box.size,
        baseline,
        {grid.scratch->total_width, scratch.height},
        scratch.ascent);
    float current_x = box.min.x + placement.min.x;
    float const total_extra_space
        = (std::max) (0.f, placement.size.x - grid.scratch->total_width);
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, grid.scratch->total_growth);
    auto const* column_data = grid.scratch->columns;
    for (alia_layout_node* child = grid_row.container.first_child;
         child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *column_data++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        alia_assign_boxes(
            ctx,
            ALIA_MAIN_AXIS_X,
            child,
            {.min = {current_x, box.min.y + placement.min.y},
             .size = {child_x.min_size + extra_space, box.size.y}},
            baseline);
        current_x += child_x.min_size + extra_space;
    }
    alia_arena_jump(&ctx->scratch, marker);
}

alia_layout_node_vtable grid_row_vtable = {
    grid_row_measure_horizontal,
    grid_row_assign_widths,
    grid_row_measure_vertical,
    grid_row_assign_boxes,
};

} // namespace alia

using namespace alia;

extern "C" {

typedef struct alia_layout_grid_scope
{
    grid_layout_node* grid = nullptr;
    grid_row_layout_node** next_row_ptr = nullptr;
} alia_layout_grid_scope;

alia_layout_grid_handle
alia_layout_grid_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_grid_scope>(ctx);
        auto& emission = ctx->layout->emission;
        grid_layout_node* node = arena_alloc<grid_layout_node>(emission.arena);
        scope.grid = node;
        scope.next_row_ptr = &node->first_row;
        *node = grid_layout_node{
            .base = {.vtable = &grid_vtable, .next_sibling = nullptr},
            .first_row = nullptr,
            .scratch = nullptr,
            .column
            = {.base
               = {.vtable = &alia::column_vtable, .next_sibling = nullptr},
               .flags = flags,
               .first_child = nullptr},
        };
        *emission.next_ptr = &node->base;
        emission.next_ptr = &node->column.first_child;
        return &scope;
    }
    return nullptr;
}

void
alia_layout_grid_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_grid_scope>(ctx);
        *scope.next_row_ptr = 0;
        auto& emission = ctx->layout->emission;
        *emission.next_ptr = 0;
        emission.next_ptr = &scope.grid->base.next_sibling;
    }
}

struct grid_row_layout_node_scope
{
    grid_row_layout_node* node;
};

void
alia_layout_grid_row_begin(
    alia_context* ctx, alia_layout_grid_handle grid, alia_layout_flags_t flags)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<grid_row_layout_node_scope>(ctx);
        auto& emission = ctx->layout->emission;
        auto* node = arena_alloc<grid_row_layout_node>(emission.arena);
        *node = grid_row_layout_node{
            .container
            = {.base = {.vtable = &grid_row_vtable, .next_sibling = nullptr},
               .flags = flags,
               .first_child = nullptr},
            .grid = grid->grid,
            .next_row = nullptr};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
        *grid->next_row_ptr = node;
        grid->next_row_ptr = &node->next_row;
    }
}

void
alia_layout_grid_row_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<grid_row_layout_node_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
