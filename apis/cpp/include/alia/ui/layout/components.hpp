#pragma once

#include <alia/abi/ui/layout/components.h>
#include <alia/context.h>
#include <alia/ui/layout/flags.hpp>

#include <utility>

namespace alia {

// COMPOSITION CONTAINERS

template<class Content>
void
row(context& ctx, Content&& content)
{
    alia_layout_row_begin(&ctx, 0);
    std::forward<Content>(content)();
    alia_layout_row_end(&ctx);
}

template<class Content>
void
row(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_row_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_row_end(&ctx);
}

template<class Content>
void
column(context& ctx, Content&& content)
{
    alia_layout_column_begin(&ctx, 0);
    std::forward<Content>(content)();
    alia_layout_column_end(&ctx);
}

template<class Content>
void
column(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_column_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_column_end(&ctx);
}

template<class Content>
void
flow(context& ctx, Content&& content)
{
    alia_layout_flow_begin(&ctx, 0);
    std::forward<Content>(content)();
    alia_layout_flow_end(&ctx);
}

template<class Content>
void
flow(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_flow_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_flow_end(&ctx);
}

template<class Content>
void
hyperflow(context& ctx, Content&& content)
{
    alia_layout_hyperflow_begin(&ctx, 0);
    std::forward<Content>(content)();
    alia_layout_hyperflow_end(&ctx);
}

template<class Content>
void
hyperflow(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_hyperflow_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_hyperflow_end(&ctx);
}

template<class Content>
void
grid(context& ctx, Content&& content)
{
    alia_layout_grid_handle handle = alia_layout_grid_begin(&ctx, 0);
    std::forward<Content>(content)(handle);
    alia_layout_grid_end(&ctx);
}

template<class Content>
void
grid(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_grid_handle handle
        = alia_layout_grid_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)(handle);
    alia_layout_grid_end(&ctx);
}

template<class Content>
void
grid_row(context& ctx, alia_layout_grid_handle grid, Content&& content)
{
    alia_layout_grid_row_begin(&ctx, grid, 0);
    std::forward<Content>(content)();
    alia_layout_grid_row_end(&ctx);
}

template<class Content>
void
grid_row(
    context& ctx,
    alia_layout_grid_handle grid,
    layout_flag_set flags,
    Content&& content)
{
    alia_layout_grid_row_begin(&ctx, grid, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_grid_row_end(&ctx);
}

// WRAPPERS

template<class Content>
void
alignment_override(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_alignment_override_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_alignment_override_end(&ctx);
}

template<class Content>
void
placement_hook(context& ctx, Content&& content)
{
    alia_layout_placement* placement
        = alia_layout_placement_hook_begin(&ctx, 0);
    std::forward<Content>(content)(*placement);
    alia_layout_placement_hook_end(&ctx);
}

template<class Content>
void
placement_hook(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_placement* placement
        = alia_layout_placement_hook_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)(*placement);
    alia_layout_placement_hook_end(&ctx);
}

template<class Content>
void
inset(context& ctx, alia_insets insets, Content&& content)
{
    alia_layout_inset_begin(&ctx, insets, 0);
    std::forward<Content>(content)();
    alia_layout_inset_end(&ctx);
}

template<class Content>
void
min_size_constraint(context& ctx, alia_vec2f min_size, Content&& content)
{
    alia_layout_min_size_begin(&ctx, min_size);
    std::forward<Content>(content)();
    alia_layout_min_size_end(&ctx);
}

template<class Content>
void
growth_override(context& ctx, float growth, Content&& content)
{
    alia_layout_growth_override_begin(&ctx, growth);
    std::forward<Content>(content)();
    alia_layout_growth_override_end(&ctx);
}

} // namespace alia
