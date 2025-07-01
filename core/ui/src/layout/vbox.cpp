#include <alia/ui/layout/vbox.hpp>

#include <alia/ui/layout/scratch.hpp>

namespace alia {

struct VBoxScratch
{
    float max_width = 0;
    float total_height = 0, total_growth = 0;
    bool has_baseline = false;
    float baseline_y = 0;
};

HorizontalRequirements
gather_vbox_x_requirements(LayoutScratchArena& scratch, LayoutNode& vbox)
{
    auto& vbox_scratch = claim_scratch<VBoxScratch>(scratch);
    for (LayoutNode* child = vbox.container.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = gather_x_requirements(scratch, *child);
        vbox_scratch.max_width
            = (std::max)(vbox_scratch.max_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = vbox_scratch.max_width + vbox.margin.x * 2,
        .growth_factor = 0};
}

HorizontalRequirements
recall_vbox_x_requirements(LayoutScratchArena& scratch, LayoutNode& vbox)
{
    auto& vbox_scratch = peek_scratch<VBoxScratch>(scratch);
    return HorizontalRequirements{
        .min_size = vbox_scratch.max_width + vbox.margin.x * 2,
        .growth_factor = 0};
}

void
assign_vbox_widths(
    LayoutScratchArena& scratch, LayoutNode& vbox, float assigned_width)
{
    auto& vbox_scratch = use_scratch<VBoxScratch>(scratch);
    assigned_width -= vbox.margin.x * 2;
    for (LayoutNode* child = vbox.container.first_child; child != nullptr;
         child = child->next_sibling)
    {
        assign_widths(scratch, *child, assigned_width);
    }
}

VerticalRequirements
gather_vbox_y_requirements(
    LayoutScratchArena& scratch, LayoutNode& vbox, float assigned_width)
{
    auto& vbox_scratch = use_scratch<VBoxScratch>(scratch);
    assigned_width -= vbox.margin.y * 2;
    for (LayoutNode* child = vbox.container.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y
            = gather_y_requirements(scratch, *child, assigned_width);
        vbox_scratch.total_height += child_y.min_size;
        vbox_scratch.total_growth += child_y.growth_factor;
        // TODO: Handle baseline.
    }
    return VerticalRequirements{
        .min_size = vbox_scratch.total_height + vbox.margin.y * 2,
        .growth_factor = 0};
}

VerticalRequirements
recall_vbox_y_requirements(LayoutScratchArena& scratch, LayoutNode& vbox)
{
    auto& vbox_scratch = peek_scratch<VBoxScratch>(scratch);
    return VerticalRequirements{
        .min_size = vbox_scratch.total_height + vbox.margin.y * 2,
        .growth_factor = 0};
}

void
assign_vbox_boxes(PlacementContext& ctx, LayoutNode& vbox, Box box)
{
    auto& vbox_scratch = use_scratch<VBoxScratch>(*ctx.scratch);
    box = apply_margin(box, vbox.margin);
    float const total_extra_space
        = (std::max)(0.f, box.size.y - vbox_scratch.total_height);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max)(0.00001f, vbox_scratch.total_growth);
    float current_y = box.pos.y;
    for (LayoutNode* child = vbox.container.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = recall_y_requirements(*ctx.scratch, *child);
        float const extra_space
            = total_extra_space * child_y.growth_factor / total_growth;
        assign_boxes(
            ctx,
            *child,
            Box{Vec2{box.pos.x, current_y},
                Vec2{box.size.x, child_y.min_size + extra_space}});
        current_y += child_y.min_size + extra_space;
    }
}

} // namespace alia
