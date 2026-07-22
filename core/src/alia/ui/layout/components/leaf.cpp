#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/styling.h>

#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

struct layout_leaf_node
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    float spacing;
    alia_layout_content_metrics content;
};

static float
leaf_effective_spacing(layout_leaf_node const& leaf)
{
    return (leaf.flags & ALIA_FLUSH) != 0 ? 0.f : leaf.spacing;
}

alia_horizontal_requirements
leaf_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    float const spacing = leaf_effective_spacing(leaf);
    return alia_horizontal_requirements{
        .min_size = leaf.content.size.x + spacing * 2,
        .growth_factor = alia_resolve_growth_factor(leaf.flags)};
}

alia_vertical_requirements
leaf_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& leaf = *reinterpret_cast<layout_leaf_node*>(node);
    float const spacing = leaf_effective_spacing(leaf);
    return alia_vertical_requirements{
        .min_size = leaf.content.size.y + spacing * 2,
        .growth_factor = alia_resolve_growth_factor(leaf.flags),
        .ascent = leaf.content.ascent,
        .descent = leaf.content.descent};
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
    float const spacing = leaf_effective_spacing(leaf);
    alia_box* placement = arena_alloc<alia_box>(ctx->arena);
    auto const padded_placement = alia_resolve_leaf_box(
        alia_fold_in_cross_axis_flags(leaf.flags, main_axis),
        box.size,
        baseline,
        leaf.content.size,
        leaf.content.ascent,
        {spacing, spacing});
    placement->min = box.min + padded_placement.min;
    placement->size = padded_placement.size;
}

alia_layout_node_vtable leaf_vtable
    = {leaf_measure_horizontal,
       leaf_measure_vertical,
       leaf_assign_boxes,
       alia_default_count_flow_emissions,
       alia_default_emit_flow_fragments,
       alia_default_read_fragment_placements};

} // namespace alia

using namespace alia;

extern "C" {

void
alia_layout_leaf_emit(
    alia_context* ctx,
    alia_layout_content_metrics content,
    alia_layout_flags_t flags)
{
    auto& emission = ctx->layout->emission;
    layout_leaf_node* new_node = arena_alloc<layout_leaf_node>(emission.arena);
    *emission.next_ptr = &new_node->base;
    emission.next_ptr = &new_node->base.next_sibling;
    *new_node = layout_leaf_node{
        .base = {.vtable = &leaf_vtable, .next_sibling = 0},
        .flags = flags,
        .spacing = alia_layout_style_active(ctx)->spacing,
        .content = content};
}

} // extern "C"
