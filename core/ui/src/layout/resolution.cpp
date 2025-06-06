#include <alia/ui/layout/resolution.hpp>

#include <alia/ui/layout/hbox.hpp>
#include <alia/ui/layout/vbox.hpp>

namespace alia {

void
resolve_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Vec2 available_space)
{
    scratch.reset();
    gather_x_requirements(specs, scratch, 1);
    scratch.reset();
    assign_widths(specs, scratch, available_space.x, 1);
    scratch.reset();
    gather_y_requirements(specs, scratch, available_space.y, 1);
    scratch.reset();
    assign_boxes(
        specs, scratch, placements, Box{Vec2{0, 0}, available_space}, 1);
    scratch.reset();
}

HorizontalRequirements
gather_x_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_x_requirements(specs, scratch, spec);
        case LayoutNodeType::VBox:
            return gather_vbox_x_requirements(specs, scratch, spec);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = spec.size.x + spec.margin.x * 2,
                .growth_factor = spec.growth_factor};
    }
}

HorizontalRequirements
recall_x_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_x_requirements(specs, scratch, spec);
        case LayoutNodeType::VBox:
            return recall_vbox_x_requirements(specs, scratch, spec);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = spec.size.x + spec.margin.x * 2,
                .growth_factor = spec.growth_factor};
    }
}

void
assign_widths(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_widths(specs, scratch, assigned_width, spec);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_widths(specs, scratch, assigned_width, spec);
            break;
        default:
        case LayoutNodeType::Leaf:
            break;
    }
}

VerticalRequirements
gather_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_y_requirements(
                specs, scratch, assigned_width, spec);
        case LayoutNodeType::VBox:
            return gather_vbox_y_requirements(
                specs, scratch, assigned_width, spec);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = spec.size.y + spec.margin.y * 2,
                .growth_factor = spec.growth_factor};
    }
}

VerticalRequirements
recall_y_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_y_requirements(specs, scratch, spec);
        case LayoutNodeType::VBox:
            return recall_vbox_y_requirements(specs, scratch, spec);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = spec.size.y + spec.margin.y * 2,
                .growth_factor = spec.growth_factor};
    }
}

void
assign_boxes(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Box box,
    LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_boxes(specs, scratch, placements, box, spec);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_boxes(specs, scratch, placements, box, spec);
            break;
        default:
        case LayoutNodeType::Leaf:
            placements[index].position = box.pos + spec.margin;
            placements[index].size = spec.size - spec.margin * 2;
            break;
    }
}

} // namespace alia
