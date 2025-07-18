#include <alia/ui/layout/inset.hpp>

#include <alia/ui/layout/resolution.hpp>
#include <alia/ui/layout/scratch.hpp>

namespace alia {

HorizontalRequirements
measure_inset_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    ALIA_ASSERT(inset.container.child_count == 1);
    auto const child_x = measure_horizontal(ctx, inset.container.first_child);
    return HorizontalRequirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

void
assign_inset_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    ALIA_ASSERT(inset.container.child_count == 1);
    assign_widths(
        ctx,
        inset.container.first_child,
        assigned_width - inset.insets.left - inset.insets.right);
}

VerticalRequirements
measure_inset_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    ALIA_ASSERT(inset.container.child_count == 1);
    auto const child_y = measure_vertical(
        ctx,
        inset.container.first_child,
        assigned_width - inset.insets.top - inset.insets.bottom);
    return VerticalRequirements{
        .min_size = child_y.min_size + inset.insets.top + inset.insets.bottom,
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent + inset.insets.top,
        .descent = child_y.descent + inset.insets.bottom};
}

void
assign_inset_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    ALIA_ASSERT(inset.container.child_count == 1);
    assign_boxes(
        ctx,
        inset.container.first_child,
        Box{.pos = box.pos + Vec2{inset.insets.left, inset.insets.top},
            .size = box.size - Vec2{inset.insets.left + inset.insets.right,
                                    inset.insets.top + inset.insets.bottom}},
        baseline - inset.insets.top);
}

HorizontalRequirements
measure_wrapped_inset_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    ALIA_ASSERT(inset.container.child_count == 1);
    auto const child_x
        = measure_wrapped_horizontal(ctx, inset.container.first_child);
    return HorizontalRequirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

WrappingRequirements
measure_wrapped_inset_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);

    ALIA_ASSERT(inset.container.child_count == 1);
    auto const child = measure_wrapped_vertical(
        ctx,
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
        requirements.interior_height += inset.insets.top;
    }

    if (has_wrapped_content(child))
    {
        requirements.last_line.height += inset.insets.bottom;
        requirements.last_line.ascent += inset.insets.bottom;
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
assign_wrapped_inset_boxes(
    PlacementContext* ctx,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    auto& inset = *reinterpret_cast<InsetLayoutNode*>(node);
    ALIA_ASSERT(inset.container.child_count == 1);
    WrappingAssignment inset_assignment = *assignment;
    inset_assignment.x_base += inset.insets.left;
    inset_assignment.line_width -= inset.insets.left + inset.insets.right;
    assign_wrapped_boxes(ctx, inset.container.first_child, &inset_assignment);
}

LayoutNodeVtable inset_vtable
    = {measure_inset_horizontal,
       assign_inset_widths,
       measure_inset_vertical,
       assign_inset_boxes,
       measure_wrapped_horizontal,
       measure_wrapped_vertical,
       assign_wrapped_boxes};

} // namespace alia
