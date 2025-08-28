#include <alia/ui/layout/modifiers/alignment_override.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

struct alignment_override_scratch
{
    horizontal_requirements horizontal;
    vertical_requirements vertical;
};

void
begin_alignment_override(
    context& ctx, layout_container_scope& scope, layout_flag_set flags)
{
    if (ctx.pass.type == pass_type::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        alignment_override_node* node
            = arena_alloc<alignment_override_node>(*layout.arena);
        *node = alignment_override_node{
            .container
            = {.base
               = {.vtable = &alignment_override_vtable, .next_sibling = 0},
               .flags = NO_FLAGS,
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

horizontal_requirements
alignment_override_measure_horizontal(
    measurement_context* ctx, layout_node* node)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = claim_scratch<alignment_override_scratch>(*ctx->scratch);
    scratch.horizontal
        = measure_horizontal(ctx, override.container.first_child);
    return scratch.horizontal;
}

void
alignment_override_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = use_scratch<alignment_override_scratch>(*ctx->scratch);
    auto const assignment = resolve_horizontal_assignment(
        adjust_flags_for_main_axis(override.flags, main_axis),
        assigned_width,
        scratch.horizontal.min_size);
    assign_widths(
        ctx, main_axis, override.container.first_child, assignment.size);
}

vertical_requirements
alignment_override_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = use_scratch<alignment_override_scratch>(*ctx->scratch);
    auto const assignment = resolve_horizontal_assignment(
        adjust_flags_for_main_axis(override.flags, main_axis),
        assigned_width,
        scratch.horizontal.min_size);
    scratch.vertical = measure_vertical(
        ctx, main_axis, override.container.first_child, assignment.size);
    return scratch.vertical;
}

void
alignment_override_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& override = *reinterpret_cast<alignment_override_node*>(node);
    auto& scratch = use_scratch<alignment_override_scratch>(*ctx->scratch);
    auto const placement = resolve_assignment(
        adjust_flags_for_main_axis(override.flags, main_axis),
        box.size,
        baseline,
        {scratch.horizontal.min_size, scratch.vertical.min_size},
        scratch.vertical.ascent);
    assign_boxes(
        ctx,
        main_axis,
        override.container.first_child,
        {.pos = box.pos + placement.pos, .size = placement.size},
        scratch.vertical.ascent);
}

layout_node_vtable alignment_override_vtable
    = {alignment_override_measure_horizontal,
       alignment_override_assign_widths,
       alignment_override_measure_vertical,
       alignment_override_assign_boxes,
       alignment_override_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
