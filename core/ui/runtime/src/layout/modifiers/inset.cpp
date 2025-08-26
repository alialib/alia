#include <alia/ui/layout/modifiers/inset.hpp>

#include <alia/ui/context.hpp>

namespace alia {

void
begin_inset(
    Context& ctx,
    LayoutContainerScope& scope,
    Insets insets,
    LayoutFlagSet flags)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        InsetLayoutNode* node = arena_alloc<InsetLayoutNode>(*layout.arena);
        *node = InsetLayoutNode{
            .container
            = {.base = {.vtable = &inset_vtable, .next_sibling = 0},
               .flags = flags,
               .first_child = 0},
            .insets = insets};
        scope.container = &node->container;
        *layout.next_ptr = &node->container.base;
        layout.next_ptr = &node->container.first_child;
    }
}

void
end_inset(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

HorizontalRequirements
inset_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    auto const child_x = measure_horizontal(ctx, inset.container.first_child);
    return HorizontalRequirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

void
inset_assign_widths(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    assign_widths(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.left - inset.insets.right);
}

VerticalRequirements
inset_measure_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    auto const child_y = measure_vertical(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.top - inset.insets.bottom);
    return VerticalRequirements{
        .min_size = child_y.min_size + inset.insets.top + inset.insets.bottom,
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent + inset.insets.top,
        .descent = child_y.descent + inset.insets.bottom};
}

void
inset_assign_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    Box box,
    float baseline)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    assign_boxes(
        ctx,
        main_axis,
        inset.container.first_child,
        Box{.pos = box.pos + Vec2{inset.insets.left, inset.insets.top},
            .size = box.size - Vec2{inset.insets.left + inset.insets.right,
                                    inset.insets.top + inset.insets.bottom}},
        baseline - inset.insets.top);
}

HorizontalRequirements
inset_measure_wrapped_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    auto const child_x
        = measure_wrapped_horizontal(ctx, inset.container.first_child);
    return HorizontalRequirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

WrappingRequirements
inset_measure_wrapped_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);

    auto const child = measure_wrapped_vertical(
        ctx,
        main_axis,
        inset.container.first_child,
        current_x_offset,
        line_width - inset.insets.left - inset.insets.right);

    WrappingRequirements requirements = child;

    if (has_first_line_content(child))
    {
        requirements.first_line.height += inset.insets.top;
        requirements.first_line.ascent += inset.insets.top;
    }
    else if (has_wrapped_content(child))
    {
        if (requirements.interior_height > 0)
        {
            requirements.interior_height += inset.insets.top;
        }
        else
        {
            requirements.last_line.height += inset.insets.top;
            requirements.last_line.ascent += inset.insets.top;
        }
    }

    if (has_wrapped_content(child))
    {
        requirements.last_line.height += inset.insets.bottom;
        requirements.last_line.descent += inset.insets.bottom;
    }
    else if (has_first_line_content(child))
    {
        requirements.first_line.height += inset.insets.bottom;
        requirements.first_line.descent += inset.insets.bottom;
    }

    requirements.end_x += inset.insets.left + inset.insets.right;

    return requirements;
}

void
inset_assign_wrapped_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    WrappingAssignment inset_assignment = *assignment;
    inset_assignment.x_base += inset.insets.left;
    inset_assignment.line_width -= inset.insets.left + inset.insets.right;
    assign_wrapped_boxes(
        ctx, main_axis, inset.container.first_child, &inset_assignment);
}

LayoutNodeVtable inset_vtable
    = {inset_measure_horizontal,
       inset_assign_widths,
       inset_measure_vertical,
       inset_assign_boxes,
       inset_measure_wrapped_horizontal,
       inset_measure_wrapped_vertical,
       inset_assign_wrapped_boxes};

} // namespace alia
