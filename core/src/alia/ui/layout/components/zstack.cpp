#include <alia/abi/ui/layout/utilities/placement.h>
#include <alia/impl/ui/layout.hpp>

#include <algorithm>

using namespace alia::operators;

namespace alia {

using zstack_layout_node = alia_layout_container;

struct zstack_scratch
{
    float max_width = 0;
    float max_height = 0;
};

alia_horizontal_requirements
zstack_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& zstack = *reinterpret_cast<zstack_layout_node*>(node);
    auto& scratch = claim_scratch<zstack_scratch>(ctx->scratch);

    for (alia_layout_node* child = zstack.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = alia_measure_horizontal(ctx, child);
        scratch.max_width = (std::max) (scratch.max_width, child_x.min_size);
    }
    return alia_horizontal_requirements{
        .min_size = scratch.max_width,
        .growth_factor = alia_resolve_growth_factor(zstack.flags)};
}

alia_vertical_requirements
zstack_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& zstack = *reinterpret_cast<zstack_layout_node*>(node);
    auto& scratch = use_scratch<zstack_scratch>(ctx->scratch);

    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(zstack.flags, main_axis),
        assigned_width,
        scratch.max_width);

    float max_height = 0.f;
    for (alia_layout_node* child = zstack.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y
            = alia_measure_vertical(ctx, main_axis, child, assignment.size);
        max_height = (std::max) (max_height, child_y.min_size);
    }
    scratch.max_height = max_height;

    // Layers share a baseline inside the stack, but the stack does not
    // participate in its parent's baseline group.
    return alia_mask_reported_vertical_requirements(
        zstack.flags,
        main_axis,
        alia_vertical_requirements{
            .min_size = scratch.max_height,
            .growth_factor = alia_resolve_growth_factor(zstack.flags),
            .ascent = 0.f,
            .descent = 0.f});
}

void
zstack_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& zstack = *reinterpret_cast<zstack_layout_node*>(node);
    auto& scratch = use_scratch<zstack_scratch>(ctx->scratch);

    auto const placement = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(zstack.flags, main_axis),
        box.size,
        baseline,
        {scratch.max_width, scratch.max_height},
        0.f);
    if ((zstack.flags & ALIA_PROVIDE_BOX) != 0)
    {
        alia_box* provided_box = arena_alloc<alia_box>(ctx->arena);
        provided_box->min = box.min + placement.min;
        provided_box->size = placement.size;
    }

    alia_box const child_box
        = {.min = box.min + placement.min, .size = placement.size};
    // The stack reports no ascent to its parent, so layers share an internal
    // baseline derived from the stack's own baseline-group flags.
    float const child_baseline = alia_resolve_baseline(
        zstack.flags, placement.size.y, 0.f, 0.f);

    for (alia_layout_node* child = zstack.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_assign_boxes(ctx, main_axis, child, child_box, child_baseline);
    }
}

alia_layout_node_vtable zstack_vtable
    = {zstack_measure_horizontal,
       zstack_measure_vertical,
       zstack_assign_boxes,
       alia_default_count_flow_emissions,
       alia_default_emit_flow_fragments,
       alia_default_read_fragment_placements};

} // namespace alia

extern "C" {

void
alia_layout_zstack_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    alia_layout_container_simple_begin(ctx, &alia::zstack_vtable, flags, 0.f);
}

void
alia_layout_zstack_end(alia_context* ctx)
{
    alia_layout_container_simple_end(ctx);
}

} // extern "C"
