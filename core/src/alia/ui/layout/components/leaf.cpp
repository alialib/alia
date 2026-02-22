#include <alia/abi/ui/layout/components.h>

#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

struct layout_leaf_node
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    float padding;
    alia_vec2f size;
};

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
    alia_box* placement = arena_alloc<alia_box>(ctx->arena);
    auto const padded_placement = alia_resolve_leaf_box(
        alia_fold_in_cross_axis_flags(leaf.flags, main_axis),
        box.size,
        baseline,
        leaf.size,
        0,
        {leaf.padding, leaf.padding});
    placement->min = box.min + padded_placement.min;
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

using namespace alia;

extern "C" {

void
alia_layout_leaf_emit(
    alia_context* ctx, alia_vec2f size, alia_layout_flags_t flags)
{
    auto& emission = ctx->layout->emission;
    layout_leaf_node* new_node = arena_alloc<layout_leaf_node>(emission.arena);
    *emission.next_ptr = &new_node->base;
    emission.next_ptr = &new_node->base.next_sibling;
    *new_node = layout_leaf_node{
        .base = {.vtable = &leaf_vtable, .next_sibling = 0},
        .flags = flags,
        .padding = ctx->style->padding,
        .size = size};
}

alia_box
alia_layout_leaf_read(alia_context* ctx)
{
    return *arena_alloc<alia_box>(*alia_layout_placement_arena(ctx));
}

} // extern "C"