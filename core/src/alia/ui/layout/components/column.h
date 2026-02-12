#pragma once

#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/layout/utilities.h>

namespace alia {

using column_layout_node = alia_layout_container;

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
