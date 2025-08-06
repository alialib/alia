#include <alia/ui/layout/alignment_override.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

struct AlignmentOverrideScratch
{
    HorizontalRequirements horizontal;
    VerticalRequirements vertical;
};

void
begin_alignment_override(
    Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        AlignmentOverrideNode* this_container
            = arena_alloc<AlignmentOverrideNode>(*layout.arena);
        scope.this_container
            = reinterpret_cast<LayoutContainer*>(this_container);
        *this_container = AlignmentOverrideNode{
            .container
            = {.base
               = {.vtable = &alignment_override_vtable, .next_sibling = 0},
               .flags = NO_FLAGS,
               .first_child = 0},
            .flags = flags};
        *layout.next_ptr = &this_container->container.base;
        layout.next_ptr = &this_container->container.first_child;
    }
}

void
end_alignment_override(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

HorizontalRequirements
alignment_override_measure_horizontal(
    MeasurementContext* ctx, LayoutNode* node)
{
    auto& override = *reinterpret_cast<AlignmentOverrideNode*>(node);
    auto& scratch = claim_scratch<AlignmentOverrideScratch>(*ctx->scratch);
    scratch.horizontal
        = measure_horizontal(ctx, override.container.first_child);
    return scratch.horizontal;
}

void
alignment_override_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& override = *reinterpret_cast<AlignmentOverrideNode*>(node);
    auto& scratch = use_scratch<AlignmentOverrideScratch>(*ctx->scratch);
    auto const assignment = resolve_horizontal_assignment(
        override.flags, assigned_width, scratch.horizontal.min_size);
    assign_widths(ctx, override.container.first_child, assignment.size);
}

VerticalRequirements
alignment_override_measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& override = *reinterpret_cast<AlignmentOverrideNode*>(node);
    auto& scratch = use_scratch<AlignmentOverrideScratch>(*ctx->scratch);
    auto const assignment = resolve_horizontal_assignment(
        override.flags, assigned_width, scratch.horizontal.min_size);
    scratch.vertical = measure_vertical(
        ctx, override.container.first_child, assignment.size);
    return scratch.vertical;
}

void
alignment_override_assign_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& override = *reinterpret_cast<AlignmentOverrideNode*>(node);
    auto& scratch = use_scratch<AlignmentOverrideScratch>(*ctx->scratch);
    auto const placement = resolve_assignment(
        override.flags,
        box.size,
        baseline,
        {scratch.horizontal.min_size, scratch.vertical.min_size},
        scratch.vertical.ascent);
    assign_boxes(
        ctx,
        override.container.first_child,
        Box{.pos = box.pos + placement.pos, .size = placement.size},
        scratch.vertical.ascent);
}

LayoutNodeVtable alignment_override_vtable
    = {alignment_override_measure_horizontal,
       alignment_override_assign_widths,
       alignment_override_measure_vertical,
       alignment_override_assign_boxes,
       alignment_override_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
