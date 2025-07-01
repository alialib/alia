#include <alia/ui/layout/resolution.hpp>

#include <alia/ui/layout/flow.hpp>
#include <alia/ui/layout/hbox.hpp>
#include <alia/ui/layout/vbox.hpp>
namespace alia {

LayoutPlacement*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space)
{
    scratch.reset();
    gather_x_requirements(scratch, root_node);
    scratch.reset();
    assign_widths(scratch, root_node, available_space.x);
    scratch.reset();
    gather_y_requirements(scratch, root_node, available_space.y);
    scratch.reset();
    LayoutPlacement* initial_placement = nullptr;
    PlacementContext ctx{&scratch, &arena, &initial_placement};
    assign_boxes(ctx, root_node, Box{Vec2{0, 0}, available_space});
    scratch.reset();
    return initial_placement;
}

HorizontalRequirements
gather_x_requirements(LayoutScratchArena& scratch, LayoutNode& node)
{
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_x_requirements(scratch, node);
        case LayoutNodeType::VBox:
            return gather_vbox_x_requirements(scratch, node);
        case LayoutNodeType::Flow:
            return gather_flow_x_requirements(scratch, node);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = node.size.x + node.margin.x * 2,
                .growth_factor = node.growth_factor};
    }
}

HorizontalRequirements
recall_x_requirements(LayoutScratchArena& scratch, LayoutNode& node)
{
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_x_requirements(scratch, node);
        case LayoutNodeType::VBox:
            return recall_vbox_x_requirements(scratch, node);
        case LayoutNodeType::Flow:
            return recall_flow_x_requirements(scratch, node);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = node.size.x + node.margin.x * 2,
                .growth_factor = node.growth_factor};
    }
}

void
assign_widths(
    LayoutScratchArena& scratch, LayoutNode& node, float assigned_width)
{
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_widths(scratch, node, assigned_width);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_widths(scratch, node, assigned_width);
            break;
        case LayoutNodeType::Flow:
            assign_flow_widths(scratch, node, assigned_width);
            break;
        default:
        case LayoutNodeType::Leaf:
            break;
    }
}

VerticalRequirements
gather_y_requirements(
    LayoutScratchArena& scratch, LayoutNode& node, float assigned_width)
{
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_y_requirements(scratch, node, assigned_width);
        case LayoutNodeType::VBox:
            return gather_vbox_y_requirements(scratch, node, assigned_width);
        case LayoutNodeType::Flow:
            return gather_flow_y_requirements(scratch, node, assigned_width);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = node.size.y + node.margin.y * 2,
                .growth_factor = node.growth_factor};
    }
}

VerticalRequirements
recall_y_requirements(LayoutScratchArena& scratch, LayoutNode& node)
{
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_y_requirements(scratch, node);
        case LayoutNodeType::VBox:
            return recall_vbox_y_requirements(scratch, node);
        case LayoutNodeType::Flow:
            return recall_flow_y_requirements(scratch, node);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = node.size.y + node.margin.y * 2,
                .growth_factor = node.growth_factor};
    }
}

void
assign_boxes(PlacementContext& ctx, LayoutNode& node, Box box)
{
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_boxes(ctx, node, box);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_boxes(ctx, node, box);
            break;
        case LayoutNodeType::Flow:
            assign_flow_boxes(ctx, node, box);
            break;
        default:
        case LayoutNodeType::Leaf: {
            LayoutPlacement* placement
                = reinterpret_cast<LayoutPlacement*>(ctx.arena->allocate(
                    sizeof(LayoutPlacement), alignof(LayoutPlacement)));
            placement->position = box.pos + node.margin;
            placement->size = node.size - node.margin * 2;
            *ctx.next_ptr = placement;
            ctx.next_ptr = &placement->next;
            break;
        }
    }
}

} // namespace alia
