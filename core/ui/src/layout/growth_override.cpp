#include <alia/ui/layout/growth_override.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/scratch.hpp>

namespace alia {

void
begin_growth_override(Context& ctx, LayoutContainerScope& scope, float growth)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        GrowthOverrideNode* this_container
            = reinterpret_cast<GrowthOverrideNode*>(layout.arena->allocate(
                sizeof(GrowthOverrideNode), alignof(GrowthOverrideNode)));
        scope.this_container
            = reinterpret_cast<LayoutContainer*>(this_container);
        *this_container = GrowthOverrideNode{
            .container
            = {.base = {.vtable = &growth_override_vtable, .next_sibling = 0},
               .flags = NO_FLAGS,
               .child_count = 0,
               .first_child = 0},
            .growth = growth};
        *layout.next_ptr = &this_container->container.base;
        layout.next_ptr = &this_container->container.first_child;
        scope.parent_container = layout.active_container;
        ++scope.parent_container->child_count;
        layout.active_container = scope.this_container;
    }
}

void
end_growth_override(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

HorizontalRequirements
growth_override_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& override = *reinterpret_cast<GrowthOverrideNode*>(node);
    ALIA_ASSERT(override.container.child_count == 1);
    auto const child_x
        = measure_horizontal(ctx, override.container.first_child);
    return HorizontalRequirements{
        .min_size = child_x.min_size, .growth_factor = override.growth};
}

void
growth_override_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& override = *reinterpret_cast<GrowthOverrideNode*>(node);
    ALIA_ASSERT(override.container.child_count == 1);
    assign_widths(ctx, override.container.first_child, assigned_width);
}

VerticalRequirements
growth_override_measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& override = *reinterpret_cast<GrowthOverrideNode*>(node);
    ALIA_ASSERT(override.container.child_count == 1);
    auto const child_y = measure_vertical(
        ctx, override.container.first_child, assigned_width);
    return VerticalRequirements{
        .min_size = child_y.min_size,
        .growth_factor = override.growth,
        .ascent = child_y.ascent,
        .descent = child_y.descent};
}

void
growth_override_assign_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& override = *reinterpret_cast<GrowthOverrideNode*>(node);
    ALIA_ASSERT(override.container.child_count == 1);
    assign_boxes(ctx, override.container.first_child, box, baseline);
}

HorizontalRequirements
growth_override_measure_wrapped_horizontal(
    MeasurementContext* ctx, LayoutNode* node)
{
    auto& override = *reinterpret_cast<GrowthOverrideNode*>(node);
    ALIA_ASSERT(override.container.child_count == 1);
    auto const child_x
        = measure_wrapped_horizontal(ctx, override.container.first_child);
    return HorizontalRequirements{
        .min_size = child_x.min_size, .growth_factor = child_x.growth_factor};
}

WrappingRequirements
growth_override_measure_wrapped_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto& override = *reinterpret_cast<GrowthOverrideNode*>(node);
    ALIA_ASSERT(override.container.child_count == 1);
    return measure_wrapped_vertical(
        ctx, override.container.first_child, current_x_offset, line_width);
}

void
growth_override_assign_wrapped_boxes(
    PlacementContext* ctx,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    auto& override = *reinterpret_cast<GrowthOverrideNode*>(node);
    ALIA_ASSERT(override.container.child_count == 1);
    assign_wrapped_boxes(ctx, override.container.first_child, assignment);
}

LayoutNodeVtable growth_override_vtable
    = {growth_override_measure_horizontal,
       growth_override_assign_widths,
       growth_override_measure_vertical,
       growth_override_assign_boxes,
       growth_override_measure_wrapped_horizontal,
       growth_override_measure_wrapped_vertical,
       growth_override_assign_wrapped_boxes};

} // namespace alia
