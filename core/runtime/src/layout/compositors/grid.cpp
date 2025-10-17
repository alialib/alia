#include <alia/layout/compositors/grid.hpp>

#include <alia/context.hpp>
#include <alia/layout/utilities.hpp>

namespace alia {

struct grid_scratch
{
    int column_count = 0;
    horizontal_requirements* columns = nullptr;
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
        max_column_count = (std::max)(max_column_count, column_count);
    }
    return max_column_count;
}

horizontal_requirements
grid_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    grid.scratch = &claim_scratch<grid_scratch>(*ctx->scratch);
    grid.scratch->column_count = count_columns(&grid);
    grid.scratch->columns = arena_array_alloc<horizontal_requirements>(
        *ctx->scratch, grid.scratch->column_count);
    // All the work of actually measuring the horizontal requirements of the
    // grid rows is done up-front, here. The grid rows will be contained within
    // the nested column below, and they will simply report these results back
    // to the column (uniformly).
    for (auto row = grid.first_row; row; row = row->next_row)
    {
        row->scratch_marker = ctx->scratch->save_state();
        claim_scratch<grid_row_scratch>(*ctx->scratch);
        int column_index = 0;
        for (layout_node* child = row->container.first_child; child != nullptr;
             child = child->next_sibling, ++column_index)
        {
            auto const child_x = measure_horizontal(ctx, child);
            grid.scratch->columns[column_index].growth_factor = (std::max)(
                grid.scratch->columns[column_index].growth_factor,
                child_x.growth_factor);
            grid.scratch->columns[column_index].min_size = (std::max)(
                grid.scratch->columns[column_index].min_size,
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
    grid.scratch_marker = ctx->scratch->save_state();
    return column_measure_horizontal(ctx, upcast<layout_node>(&grid.column));
}

void
grid_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    ctx->scratch->restore_state(grid.scratch_marker);
    column_assign_widths(
        ctx, main_axis, upcast<layout_node>(&grid.column), assigned_width);
}

vertical_requirements
grid_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    ctx->scratch->restore_state(grid.scratch_marker);
    return column_measure_vertical(
        ctx, main_axis, upcast<layout_node>(&grid.column), assigned_width);
}

void
grid_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& grid = *reinterpret_cast<grid_layout_node*>(node);
    ctx->scratch->restore_state(grid.scratch_marker);
    column_assign_boxes(
        ctx, main_axis, upcast<layout_node>(&grid.column), box, baseline);
}

layout_node_vtable grid_vtable
    = {grid_measure_horizontal,
       grid_assign_widths,
       grid_measure_vertical,
       grid_assign_boxes,
       grid_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

horizontal_requirements
grid_row_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = ctx->scratch->save_state();
    ctx->scratch->restore_state(grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(*ctx->scratch);
    ctx->scratch->restore_state(marker);
    return horizontal_requirements{
        .min_size = grid_row.grid->scratch->total_width,
        .growth_factor = resolve_growth_factor(grid_row.container.flags)};
}

void
grid_row_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = ctx->scratch->save_state();
    ctx->scratch->restore_state(grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(*ctx->scratch);
    // TODO: Implement.
    ctx->scratch->restore_state(marker);
}

vertical_requirements
grid_row_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = ctx->scratch->save_state();
    ctx->scratch->restore_state(grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(*ctx->scratch);
    auto& grid = *grid_row.grid;
    auto const placement = resolve_horizontal_assignment(
        adjust_flags_for_main_axis(grid_row.container.flags, main_axis),
        assigned_width,
        grid.scratch->total_width);
    float const total_extra_space
        = (std::max)(0.f, placement.size - grid.scratch->total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, grid.scratch->total_growth);
    float height = 0, ascent = 0, descent = 0;
    auto const* column_data = grid.scratch->columns;
    for (layout_node* child = grid_row.container.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *column_data++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y = measure_vertical(
            ctx, MAIN_AXIS_X, child, child_x.min_size + extra_space);
        height = (std::max)(height, child_y.min_size);
        ascent = (std::max)(ascent, child_y.ascent);
        descent = (std::max)(descent, child_y.descent);
    }
    scratch.height = height;
    scratch.ascent = ascent;
    ctx->scratch->restore_state(marker);
    return vertical_requirements{
        .min_size = (std::max)(height, ascent + descent),
        .growth_factor = resolve_growth_factor(grid_row.container.flags),
        .ascent = ascent,
        .descent = descent};
}

void
grid_row_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& grid_row = *reinterpret_cast<grid_row_layout_node*>(node);
    auto const marker = ctx->scratch->save_state();
    ctx->scratch->restore_state(grid_row.scratch_marker);
    auto& scratch = use_scratch<grid_row_scratch>(*ctx->scratch);
    auto& grid = *grid_row.grid;
    auto const placement = resolve_assignment(
        adjust_flags_for_main_axis(grid_row.container.flags, main_axis),
        box.size,
        baseline,
        vec2{grid.scratch->total_width, scratch.height},
        scratch.ascent);
    float current_x = box.pos.x + placement.pos.x;
    float const total_extra_space
        = (std::max)(0.f, placement.size.x - grid.scratch->total_width);
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, grid.scratch->total_growth);
    auto const* column_data = grid.scratch->columns;
    for (layout_node* child = grid_row.container.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *column_data++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_boxes(
            ctx,
            MAIN_AXIS_X,
            child,
            {.pos = vec2{current_x, box.pos.y + placement.pos.y},
             .size = vec2{child_x.min_size + extra_space, box.size.y}},
            baseline);
        current_x += child_x.min_size + extra_space;
    }
    ctx->scratch->restore_state(marker);
}

layout_node_vtable grid_row_vtable = {
    grid_row_measure_horizontal,
    grid_row_assign_widths,
    grid_row_measure_vertical,
    grid_row_assign_boxes,
};

void
begin_grid(context& ctx, grid_scope& scope, layout_flag_set flags)
{
    if (ctx.pass.type == pass_type::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        grid_layout_node* node = arena_alloc<grid_layout_node>(*layout.arena);
        scope.grid = node;
        scope.next_row_ptr = &node->first_row;
        *node = grid_layout_node{
            .base = {.vtable = &grid_vtable, .next_sibling = nullptr},
            .first_row = nullptr,
            .scratch = nullptr,
            .column
            = {.base = {.vtable = &column_vtable, .next_sibling = nullptr},
               .flags = flags,
               .first_child = nullptr},
        };
        *layout.next_ptr = &node->base;
        layout.next_ptr = &node->column.first_child;
    }
}

void
end_grid(context& ctx, grid_scope& scope)
{
    if (ctx.pass.type == pass_type::Refresh)
    {
        *scope.next_row_ptr = 0;
        auto& layout = ctx.pass.refresh.layout_emission;
        *layout.next_ptr = 0;
        layout.next_ptr = &scope.grid->base.next_sibling;
    }
}

void
begin_grid_row(
    context& ctx,
    grid_handle grid,
    layout_container_scope& scope,
    layout_flag_set flags)
{
    if (ctx.pass.type == pass_type::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        grid_row_layout_node* node
            = arena_alloc<grid_row_layout_node>(*layout.arena);
        scope.container = &node->container;
        *node = grid_row_layout_node{
            .container
            = {.base = {.vtable = &grid_row_vtable, .next_sibling = nullptr},
               .flags = flags,
               .first_child = nullptr},
            .grid = grid->grid,
            .next_row = nullptr};
        *layout.next_ptr = &node->container.base;
        layout.next_ptr = &node->container.first_child;
        *grid->next_row_ptr = node;
        grid->next_row_ptr = &node->next_row;
    }
}

void
end_grid_row(context& ctx, layout_container_scope& scope)
{
    if (ctx.pass.type == pass_type::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        *layout.next_ptr = nullptr;
        layout.next_ptr = &scope.container->base.next_sibling;
    }
}

} // namespace alia
