#include <alia/ui/layout/padding.hpp>

#include <alia/ui/layout/scratch.hpp>

namespace alia {

HorizontalRequirements
measure_padding_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& padding_node = *reinterpret_cast<PaddingLayoutNode*>(node);
    ALIA_ASSERT(padding_node.container.child_count == 1);
    MeasurementContext child_ctx = *ctx;
    child_ctx.padding = padding_node.padding;
    return measure_horizontal(&child_ctx, padding_node.container.first_child);
}

void
assign_padding_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& padding_node = *reinterpret_cast<PaddingLayoutNode*>(node);
    ALIA_ASSERT(padding_node.container.child_count == 1);
    MeasurementContext child_ctx = *ctx;
    child_ctx.padding = padding_node.padding;
    assign_widths(
        &child_ctx, padding_node.container.first_child, assigned_width);
}

VerticalRequirements
measure_padding_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& padding_node = *reinterpret_cast<PaddingLayoutNode*>(node);
    ALIA_ASSERT(padding_node.container.child_count == 1);
    MeasurementContext child_ctx = *ctx;
    child_ctx.padding = padding_node.padding;
    return measure_vertical(
        &child_ctx, padding_node.container.first_child, assigned_width);
}

void
assign_padding_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& padding_node = *reinterpret_cast<PaddingLayoutNode*>(node);
    ALIA_ASSERT(padding_node.container.child_count == 1);
    PlacementContext child_ctx = *ctx;
    child_ctx.padding = padding_node.padding;
    assign_boxes(
        &child_ctx, padding_node.container.first_child, box, baseline);
    ctx->next_ptr = child_ctx.next_ptr;
}

HorizontalRequirements
measure_padding_wrapped_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& padding_node = *reinterpret_cast<PaddingLayoutNode*>(node);
    ALIA_ASSERT(padding_node.container.child_count == 1);
    MeasurementContext child_ctx = *ctx;
    child_ctx.padding = padding_node.padding;
    return measure_wrapped_horizontal(
        &child_ctx, padding_node.container.first_child);
}

WrappingRequirements
measure_padding_wrapped_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto& padding_node = *reinterpret_cast<PaddingLayoutNode*>(node);
    ALIA_ASSERT(padding_node.container.child_count == 1);
    MeasurementContext child_ctx = *ctx;
    child_ctx.padding = padding_node.padding;
    return measure_wrapped_vertical(
        &child_ctx,
        padding_node.container.first_child,
        current_x_offset,
        line_width);
}

void
assign_padding_wrapped_boxes(
    PlacementContext* ctx,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    auto& padding_node = *reinterpret_cast<PaddingLayoutNode*>(node);
    ALIA_ASSERT(padding_node.container.child_count == 1);
    PlacementContext child_ctx = *ctx;
    child_ctx.padding = padding_node.padding;
    assign_wrapped_boxes(
        &child_ctx, padding_node.container.first_child, assignment);
    ctx->next_ptr = child_ctx.next_ptr;
}

LayoutNodeVtable padding_vtable
    = {measure_padding_horizontal,
       assign_padding_widths,
       measure_padding_vertical,
       assign_padding_boxes,
       measure_padding_wrapped_horizontal,
       measure_padding_wrapped_vertical,
       assign_padding_wrapped_boxes};

} // namespace alia
