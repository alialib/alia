#ifndef ALIA_ABI_UI_LAYOUT_UTILITIES_DISPATCH_H
#define ALIA_ABI_UI_LAYOUT_UTILITIES_DISPATCH_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/protocol.h>

ALIA_EXTERN_C_BEGIN

static inline alia_horizontal_requirements
alia_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    return node->vtable->measure_horizontal(ctx, node);
}

static inline alia_vertical_requirements
alia_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    return node->vtable->measure_vertical(
        ctx, main_axis, node, assigned_width);
}

static inline void
alia_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    node->vtable->assign_boxes(ctx, main_axis, node, box, baseline);
}

static inline alia_flow_emission_counts
alia_flow_emission_counts_add(
    alia_flow_emission_counts a, alia_flow_emission_counts b)
{
    return alia_flow_emission_counts{
        .fragment_count = a.fragment_count + b.fragment_count};
}

static inline alia_flow_emission_counts
alia_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    return node->vtable->count_flow_emissions(ctx, node);
}

static inline void
alia_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    node->vtable->emit_flow_fragments(ctx, node, emitter);
}

static inline void
alia_layout_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    return node->vtable->read_fragment_placements(ctx, node, reader);
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_UTILITIES_DISPATCH_H
