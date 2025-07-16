#include <alia/ui/layout/vbox.hpp>

#include <alia/ui/layout/scratch.hpp>

#include <iostream>

namespace alia {

struct VBoxScratch
{
    float max_width = 0;
    float total_height = 0, total_growth = 0;
    bool has_baseline = false;
    float baseline_y = 0;
};

HorizontalRequirements
measure_vbox_horizontal(LayoutScratchArena* scratch, LayoutNode* node)
{
    auto& vbox = *reinterpret_cast<VBoxLayoutNode*>(node);
    auto& vbox_scratch = claim_scratch<VBoxScratch>(*scratch);
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch->allocate(
            vbox.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = vbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(scratch, child);
        vbox_scratch.max_width
            = (std::max)(vbox_scratch.max_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = vbox_scratch.max_width, .growth_factor = 0};
}

void
assign_vbox_widths(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& vbox = *reinterpret_cast<VBoxLayoutNode*>(node);
    auto& vbox_scratch = use_scratch<VBoxScratch>(*scratch);
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch->allocate(
            vbox.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = vbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        assign_widths(scratch, child, assigned_width);
    }
}

VerticalRequirements
measure_vbox_vertical(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& vbox = *reinterpret_cast<VBoxLayoutNode*>(node);
    auto& vbox_scratch = use_scratch<VBoxScratch>(*scratch);
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch->allocate(
            vbox.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = vbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = measure_vertical(scratch, child, assigned_width);
        *y_requirements++ = child_y;
        vbox_scratch.total_height += child_y.min_size;
        vbox_scratch.total_growth += child_y.growth_factor;
        // TODO: Handle baseline.
    }
    return VerticalRequirements{
        .min_size = vbox_scratch.total_height, .growth_factor = 0};
}

void
assign_vbox_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& vbox = *reinterpret_cast<VBoxLayoutNode*>(node);
    auto& vbox_scratch = use_scratch<VBoxScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(ctx->scratch->allocate(
            vbox.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    // TODO: Handle baseline.
    float const total_extra_space
        = (std::max)(0.f, box.size.y - vbox_scratch.total_height);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max)(0.00001f, vbox_scratch.total_growth);
    float current_y = box.pos.y;
    for (LayoutNode* child = vbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = *y_requirements++;
        float const extra_space
            = total_extra_space * child_y.growth_factor / total_growth;
        assign_boxes(
            ctx,
            child,
            Box{Vec2{box.pos.x, current_y},
                Vec2{box.size.x, child_y.min_size + extra_space}},
            child_y.ascent);
        current_y += child_y.min_size + extra_space;
    }
}

LayoutNodeVtable vbox_vtable
    = {measure_vbox_horizontal,
       assign_vbox_widths,
       measure_vbox_vertical,
       assign_vbox_boxes,
       default_measure_wrapped_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
