#include <alia/ui/layout/modifiers/min_size.hpp>

#include <alia/ui/context.hpp>

namespace alia {

void
begin_min_size_constraint(
    Context& ctx, LayoutContainerScope& scope, Vec2 min_size)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        MinSizeNode* node = arena_alloc<MinSizeNode>(*layout.arena);
        *node = MinSizeNode{
            .container
            = {.base = {.vtable = &min_size_vtable, .next_sibling = 0},
               .flags = NO_FLAGS,
               .first_child = 0},
            .min_size = min_size};
        scope.container = &node->container;
        *layout.next_ptr = &node->container.base;
        layout.next_ptr = &node->container.first_child;
    }
}

void
end_min_size_constraint(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

HorizontalRequirements
min_size_measure_horizontal(MeasurementContext* ctx, LayoutNode* base_node)
{
    auto& node = *reinterpret_cast<MinSizeNode*>(base_node);
    auto const child_x = measure_horizontal(ctx, node.container.first_child);
    return HorizontalRequirements{
        .min_size = std::max(node.min_size.x, child_x.min_size),
        .growth_factor = child_x.growth_factor};
}

void
min_size_assign_widths(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<MinSizeNode*>(base_node);
    assign_widths(ctx, main_axis, node.container.first_child, assigned_width);
}

VerticalRequirements
min_size_measure_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<MinSizeNode*>(base_node);
    auto const child_y = measure_vertical(
        ctx, main_axis, node.container.first_child, assigned_width);
    return VerticalRequirements{
        .min_size = std::max(node.min_size.y, child_y.min_size),
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent,
        .descent = child_y.descent};
}

void
min_size_assign_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* base_node,
    Box box,
    float baseline)
{
    auto& node = *reinterpret_cast<MinSizeNode*>(base_node);
    assign_boxes(ctx, main_axis, node.container.first_child, box, baseline);
}

HorizontalRequirements
min_size_measure_wrapped_horizontal(
    MeasurementContext* ctx, LayoutNode* base_node)
{
    auto& node = *reinterpret_cast<MinSizeNode*>(base_node);
    auto const child_x
        = measure_wrapped_horizontal(ctx, node.container.first_child);
    return HorizontalRequirements{
        .min_size = std::max(node.min_size.x, child_x.min_size),
        .growth_factor = child_x.growth_factor};
}

WrappingRequirements
min_size_measure_wrapped_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* base_node,
    float current_x_offset,
    float line_width)
{
    auto& node = *reinterpret_cast<MinSizeNode*>(base_node);
    return measure_wrapped_vertical(
        ctx,
        main_axis,
        node.container.first_child,
        current_x_offset,
        line_width);
}

void
min_size_assign_wrapped_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* base_node,
    WrappingAssignment const* assignment)
{
    auto& node = *reinterpret_cast<MinSizeNode*>(base_node);
    assign_wrapped_boxes(
        ctx, main_axis, node.container.first_child, assignment);
}

LayoutNodeVtable min_size_vtable
    = {min_size_measure_horizontal,
       min_size_assign_widths,
       min_size_measure_vertical,
       min_size_assign_boxes,
       min_size_measure_wrapped_horizontal,
       min_size_measure_wrapped_vertical,
       min_size_assign_wrapped_boxes};

} // namespace alia
