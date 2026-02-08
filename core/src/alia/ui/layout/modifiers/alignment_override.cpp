#include <alia/layout/modifiers/alignment_override.hpp>

#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/style.h>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/layout/container.hpp>
#include <alia/layout/utilities.hpp>

using namespace alia::operators;

namespace alia {

struct alignment_override_scratch
{
    alia_horizontal_requirements horizontal;
    alia_vertical_requirements vertical;
};

void
begin_alignment_override(
    context& ctx, layout_container_scope& scope, alia_layout_flags_t flags)
{
    if (is_refresh_event(ctx))
    {
        auto& layout = as_refresh_event(ctx).layout_emission;
        alignment_override_node* node
            = arena_alloc<alignment_override_node>(*layout.arena);
        *node = alignment_override_node{
            .container
            = {.base
               = {.vtable = &alignment_override_vtable, .next_sibling = 0},
               .flags = 0,
               .first_child = 0},
            .flags = flags};
        scope.container = &node->container;
        *layout.next_ptr = &node->container.base;
        layout.next_ptr = &node->container.first_child;
    }
}

void
end_alignment_override(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

alia_horizontal_requirements
alignment_override_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = claim_scratch<alignment_override_scratch>(*ctx->scratch);
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
    auto& scratch = use_scratch<alignment_override_scratch>(*ctx->scratch);
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
    auto& scratch = use_scratch<alignment_override_scratch>(*ctx->scratch);
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
    auto& scratch = use_scratch<alignment_override_scratch>(*ctx->scratch);
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
