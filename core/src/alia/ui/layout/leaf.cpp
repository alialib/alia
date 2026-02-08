#include <alia/layout/leaf.hpp>

#include <alia/layout/utilities.hpp>

using namespace alia::operators;

namespace alia {

alia_horizontal_requirements
leaf_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    return alia_horizontal_requirements{
        .min_size = leaf.size.x + leaf.padding * 2,
        .growth_factor = alia_resolve_growth_factor(leaf.flags)};
}

void
leaf_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
}

alia_vertical_requirements
leaf_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    return alia_vertical_requirements{
        .min_size = leaf.size.y + leaf.padding * 2,
        .growth_factor = alia_resolve_growth_factor(leaf.flags),
        .ascent = 0,
        .descent = 0};
}

void
leaf_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    leaf_layout_placement* placement
        = arena_alloc<leaf_layout_placement>(*ctx->arena);
    auto const padded_placement = alia_resolve_leaf_box(
        alia_fold_in_cross_axis_flags(leaf.flags, main_axis),
        box.size,
        baseline,
        leaf.size,
        0,
        {leaf.padding, leaf.padding});
    placement->position = box.min + padded_placement.min;
    placement->size = padded_placement.size;
}

alia_layout_node_vtable leaf_vtable
    = {leaf_measure_horizontal,
       leaf_assign_widths,
       leaf_measure_vertical,
       leaf_assign_boxes,
       leaf_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
