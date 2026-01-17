#include <alia/layout/leaf.hpp>

#include <alia/layout/utilities.hpp>

using namespace alia::operators;

namespace alia {

horizontal_requirements
leaf_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    return horizontal_requirements{
        .min_size = leaf.size.x + leaf.padding * 2,
        .growth_factor = resolve_growth_factor(leaf.flags)};
}

void
leaf_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
}

vertical_requirements
leaf_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    return vertical_requirements{
        .min_size = leaf.size.y + leaf.padding * 2,
        .growth_factor = resolve_growth_factor(leaf.flags),
        .ascent = 0,
        .descent = 0};
}

void
leaf_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    leaf_layout_placement* placement
        = arena_alloc<leaf_layout_placement>(*ctx->arena);
    auto const padded_placement = resolve_padded_assignment(
        adjust_flags_for_main_axis(leaf.flags, main_axis),
        box.size,
        baseline,
        leaf.size,
        0,
        leaf.padding);
    placement->position = box.min + padded_placement.min;
    placement->size = padded_placement.size;
}

layout_node_vtable leaf_vtable
    = {leaf_measure_horizontal,
       leaf_assign_widths,
       leaf_measure_vertical,
       leaf_assign_boxes,
       leaf_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
