#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/style.h>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

struct alignment_override_node
{
    alia_layout_container container;
    alia_layout_flags_t flags;
};

struct alignment_override_scratch
{
    alia_horizontal_requirements horizontal;
    alia_vertical_requirements vertical;
};

alia_horizontal_requirements
alignment_override_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = claim_scratch<alignment_override_scratch>(ctx->scratch);
    scratch.horizontal
        = alia_measure_horizontal(ctx, override.container.first_child);
    return scratch.horizontal;
}

void
alignment_override_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = use_scratch<alignment_override_scratch>(ctx->scratch);
    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(override.flags, main_axis),
        assigned_width,
        scratch.horizontal.min_size);
    alia_assign_widths(
        ctx, main_axis, override.container.first_child, assignment.size);
}

alia_vertical_requirements
alignment_override_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = use_scratch<alignment_override_scratch>(ctx->scratch);
    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(override.flags, main_axis),
        assigned_width,
        scratch.horizontal.min_size);
    scratch.vertical = alia_measure_vertical(
        ctx, main_axis, override.container.first_child, assignment.size);
    return scratch.vertical;
}

void
alignment_override_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = use_scratch<alignment_override_scratch>(ctx->scratch);
    auto const placement = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(override.flags, main_axis),
        box.size,
        baseline,
        {scratch.horizontal.min_size, scratch.vertical.min_size},
        scratch.vertical.ascent);
    alia_assign_boxes(
        ctx,
        main_axis,
        override.container.first_child,
        {.min = box.min + placement.min, .size = placement.size},
        scratch.vertical.ascent);
}

alia_layout_node_vtable alignment_override_vtable
    = {alignment_override_measure_horizontal,
       alignment_override_assign_widths,
       alignment_override_measure_vertical,
       alignment_override_assign_boxes,
       alignment_override_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

} // namespace alia

using namespace alia;

extern "C" {

struct alia_layout_alignment_override_scope
{
    alignment_override_node* node;
};

void
alia_layout_alignment_override_begin(
    alia_context* ctx, alia_layout_flags_t flags)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_alignment_override_scope>(ctx);
        auto* node = arena_alloc<alignment_override_node>(
            ctx->layout->emission.arena);
        *node = alignment_override_node{
            .container
            = {.base
               = {.vtable = &alignment_override_vtable, .next_sibling = 0},
               .flags = 0,
               .first_child = 0},
            .flags = flags};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
    }
}

void
alia_layout_alignment_override_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_alignment_override_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
