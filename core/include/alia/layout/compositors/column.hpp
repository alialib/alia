#pragma once

#include <utility>

#include <alia/abi/ui/layout/flags.h>
#include <alia/layout/container.hpp>

// TODO: Use forward declarations once those are sorted out.
#include <alia/context.hpp>

namespace alia {

using column_layout_node = layout_container;

void
begin_column(
    context& ctx, layout_container_scope& scope, alia_layout_flags_t flags);

void
end_column(context& ctx, layout_container_scope& scope);

extern alia_layout_node_vtable column_vtable;

alia_horizontal_requirements
column_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node);

void
column_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width);

alia_vertical_requirements
column_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width);

void
column_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline);

} // namespace alia
