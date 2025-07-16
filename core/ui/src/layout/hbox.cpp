#include <alia/ui/layout/hbox.hpp>

#include <alia/ui/layout/scratch.hpp>

namespace alia {

struct HBoxScratch
{
    float total_width = 0, total_growth = 0;
    float baseline = 0;
};

HorizontalRequirements
measure_hbox_horizontal(LayoutScratchArena* scratch, LayoutNode* node)
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
        auto const child_x = measure_horizontal(scratch, child);
        *x_requirements++ = child_x;
        hbox_scratch.total_width += child_x.min_size;
        hbox_scratch.total_growth += child_x.growth_factor;
    }
    return HorizontalRequirements{
        .min_size = hbox_scratch.total_width,
        .growth_factor = node->growth_factor};
}

void
assign_hbox_widths(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = use_scratch<HBoxScratch>(*scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    auto const placement = resolve_axis_assignment(
        node->alignment, assigned_width, 0, hbox_scratch.total_width, 0);
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
measure_hbox_vertical(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = use_scratch<HBoxScratch>(*scratch);
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
    float height = 0, ascent = 0, descent = 0;
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y
            = measure_vertical(scratch, child, child_x.min_size + extra_space);
        height = (std::max)(height, child_y.min_size);
        ascent = (std::max)(ascent, child_y.ascent);
        descent = (std::max)(descent, child_y.descent);
    }
    hbox_scratch.baseline = ascent;
    return VerticalRequirements{
        .min_size = (std::max)(height, ascent + descent),
        .growth_factor = 0,
        .ascent = ascent,
        .descent = descent};
}

void
assign_hbox_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = use_scratch<HBoxScratch>(*ctx->scratch);
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
                Vec2{child_x.min_size + extra_space, box.size.y}},
            hbox_scratch.baseline);
        current_x += child_x.min_size + extra_space;
    }
}

LayoutNodeVtable hbox_vtable
    = {measure_hbox_horizontal,
       assign_hbox_widths,
       measure_hbox_vertical,
       assign_hbox_boxes,
       default_measure_wrapped_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
