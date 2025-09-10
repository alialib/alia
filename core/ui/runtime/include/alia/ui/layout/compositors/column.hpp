#pragma once

#include <utility>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct context;

using column_layout_node = layout_container;

void
begin_column(
    context& ctx, layout_container_scope& scope, layout_flag_set flags);

void
end_column(context& ctx, layout_container_scope& scope);

template<class Content>
void
column(context& ctx, Content&& content)
{
    layout_container_scope scope;
    begin_column(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

template<class Content>
void
column(context& ctx, layout_flag_set flags, Content&& content)
{
    layout_container_scope scope;
    begin_column(ctx, scope, flags);
    std::forward<Content>(content)();
    end_column(ctx, scope);
}

extern layout_node_vtable column_vtable;

horizontal_requirements
column_measure_horizontal(measurement_context* ctx, layout_node* node);

void
column_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width);

vertical_requirements
column_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width);

void
column_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline);

} // namespace alia
