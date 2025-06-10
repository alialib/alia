#include <alia/ui/layout/resolution.hpp>

#include <alia/ui/layout/flow.hpp>
#include <alia/ui/layout/hbox.hpp>
#include <alia/ui/layout/vbox.hpp>
namespace alia {

void
resolve_layout(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Vec2 available_space)
{
    scratch.reset();
    gather_x_requirements(nodes, scratch, 1);
    scratch.reset();
    assign_widths(nodes, scratch, available_space.x, 1);
    scratch.reset();
    gather_y_requirements(nodes, scratch, available_space.y, 1);
    scratch.reset();
    assign_boxes(
        nodes, scratch, placements, Box{Vec2{0, 0}, available_space}, 1);
    scratch.reset();
}

HorizontalRequirements
gather_x_requirements(
    LayoutNode const* nodes, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& node = nodes[index];
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_x_requirements(nodes, scratch, node);
        case LayoutNodeType::VBox:
            return gather_vbox_x_requirements(nodes, scratch, node);
        case LayoutNodeType::Flow:
            return gather_flow_x_requirements(nodes, scratch, node);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = node.size.x + node.margin.x * 2,
                .growth_factor = node.growth_factor};
    }
}

HorizontalRequirements
recall_x_requirements(
    LayoutNode const* nodes, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& node = nodes[index];
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_x_requirements(nodes, scratch, node);
        case LayoutNodeType::VBox:
            return recall_vbox_x_requirements(nodes, scratch, node);
        case LayoutNodeType::Flow:
            return recall_flow_x_requirements(nodes, scratch, node);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = node.size.x + node.margin.x * 2,
                .growth_factor = node.growth_factor};
    }
}

void
assign_widths(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index)
{
    auto& node = nodes[index];
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_widths(nodes, scratch, assigned_width, node);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_widths(nodes, scratch, assigned_width, node);
            break;
        case LayoutNodeType::Flow:
            assign_flow_widths(nodes, scratch, assigned_width, node);
            break;
        default:
        case LayoutNodeType::Leaf:
            break;
    }
}

VerticalRequirements
gather_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index)
{
    auto& node = nodes[index];
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_y_requirements(
                nodes, scratch, assigned_width, node);
        case LayoutNodeType::VBox:
            return gather_vbox_y_requirements(
                nodes, scratch, assigned_width, node);
        case LayoutNodeType::Flow:
            return gather_flow_y_requirements(
                nodes, scratch, assigned_width, node);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = node.size.y + node.margin.y * 2,
                .growth_factor = node.growth_factor};
    }
}

VerticalRequirements
recall_y_requirements(
    LayoutNode const* nodes, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& node = nodes[index];
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_y_requirements(nodes, scratch, node);
        case LayoutNodeType::VBox:
            return recall_vbox_y_requirements(nodes, scratch, node);
        case LayoutNodeType::Flow:
            return recall_flow_y_requirements(nodes, scratch, node);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = node.size.y + node.margin.y * 2,
                .growth_factor = node.growth_factor};
    }
}

void
assign_boxes(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Box box,
    LayoutIndex index)
{
    auto& node = nodes[index];
    switch (node.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_boxes(nodes, scratch, placements, box, node);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_boxes(nodes, scratch, placements, box, node);
            break;
        case LayoutNodeType::Flow:
            assign_flow_boxes(nodes, scratch, placements, box, node);
            break;
        default:
        case LayoutNodeType::Leaf:
            placements[index].position = box.pos + node.margin;
            placements[index].size = node.size - node.margin * 2;
            break;
    }
}

} // namespace alia
