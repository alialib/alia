#include <alia/ui/layout/hbox.hpp>

#include <alia/ui/layout/scratch.hpp>

namespace alia {

struct HBoxScratch
{
    float total_width = 0, total_growth = 0;
    bool has_baseline = false;
    float height = 0, ascent = 0, descent = 0;
};

HorizontalRequirements
gather_hbox_x_requirements(LayoutScratchArena* scratch, LayoutNode* node)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = claim_scratch<HBoxScratch>(*scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = gather_x_requirements(scratch, child);
        *x_requirements++ = child_x;
        hbox_scratch.total_width += child_x.min_size;
        hbox_scratch.total_growth += child_x.growth_factor;
    }
    return HorizontalRequirements{
        .min_size = hbox_scratch.total_width, .growth_factor = 0};
}

void
assign_hbox_widths(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = claim_scratch<HBoxScratch>(*scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    float const total_extra_space
        = (std::max)(0.f, assigned_width - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_widths(scratch, child, child_x.min_size + extra_space);
    }
}

VerticalRequirements
gather_hbox_y_requirements(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = claim_scratch<HBoxScratch>(*scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    // TODO: Stop repeating this logic everywhere.
    float const total_extra_space
        = (std::max)(0.f, assigned_width - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y = gather_y_requirements(
            scratch, child, child_x.min_size + extra_space);
        hbox_scratch.height
            = (std::max)(hbox_scratch.height, child_y.min_size);
        if (child_y.has_baseline)
        {
            hbox_scratch.ascent
                = (std::max)(hbox_scratch.ascent, child_y.baseline_offset);
            hbox_scratch.descent = (std::max)(
                hbox_scratch.descent,
                child_y.min_size - child_y.baseline_offset);
            hbox_scratch.has_baseline = true;
        }
    }
    hbox_scratch.height = (std::max)(
        hbox_scratch.height, hbox_scratch.ascent + hbox_scratch.descent);
    return VerticalRequirements{
        .min_size = hbox_scratch.height,
        .growth_factor = 0,
        .has_baseline = hbox_scratch.has_baseline,
        .baseline_offset = hbox_scratch.ascent};
}

void
assign_hbox_boxes(PlacementContext* ctx, LayoutNode* node, Box box)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = claim_scratch<HBoxScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(ctx->scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    float const total_extra_space
        = (std::max)(0.f, box.size.x - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    float current_x = box.pos.x;
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_boxes(
            ctx,
            child,
            Box{Vec2{current_x, box.pos.y},
                Vec2{child_x.min_size + extra_space, box.size.y}});
        current_x += child_x.min_size + extra_space;
    }
}

LayoutNodeVtable hbox_vtable = {
    gather_hbox_x_requirements,
    assign_hbox_widths,
    gather_hbox_y_requirements,
    assign_hbox_boxes,
};

} // namespace alia
