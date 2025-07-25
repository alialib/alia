#include <alia/ui/layout/hbox.hpp>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/scratch.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

void
begin_hbox(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags)
{
    begin_container(ctx, scope, &hbox_vtable, flags);
}

void
end_hbox(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

struct HBoxScratch
{
    float total_width = 0, total_growth = 0;
    float height = 0, ascent = 0;
};

HorizontalRequirements
hbox_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = claim_scratch<HBoxScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(ctx->scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        *x_requirements++ = child_x;
        hbox_scratch.total_width += child_x.min_size;
        hbox_scratch.total_growth += child_x.growth_factor;
    }
    return HorizontalRequirements{
        .min_size = hbox_scratch.total_width,
        .growth_factor = resolve_growth_factor(hbox.flags)};
}

void
hbox_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = use_scratch<HBoxScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(ctx->scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    auto const placement = resolve_horizontal_assignment(
        hbox.flags, assigned_width, hbox_scratch.total_width);
    float const total_extra_space
        = (std::max)(0.f, placement.size - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_widths(ctx, child, child_x.min_size + extra_space);
    }
}

VerticalRequirements
hbox_measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = use_scratch<HBoxScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(ctx->scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    // TODO: Stop repeating this logic everywhere.
    auto const placement = resolve_horizontal_assignment(
        hbox.flags, assigned_width, hbox_scratch.total_width);
    float const total_extra_space
        = (std::max)(0.f, placement.size - hbox_scratch.total_width);
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
            = measure_vertical(ctx, child, child_x.min_size + extra_space);
        height = (std::max)(height, child_y.min_size);
        ascent = (std::max)(ascent, child_y.ascent);
        descent = (std::max)(descent, child_y.descent);
    }
    hbox_scratch.height = height;
    hbox_scratch.ascent = ascent;
    return VerticalRequirements{
        .min_size = (std::max)(height, ascent + descent),
        .growth_factor = 0,
        .ascent = ascent,
        .descent = descent};
}

void
hbox_assign_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& hbox = *reinterpret_cast<HBoxLayoutNode*>(node);
    auto& hbox_scratch = use_scratch<HBoxScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(ctx->scratch->allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    auto const placement = resolve_assignment(
        hbox.flags,
        box.size,
        baseline,
        Vec2{hbox_scratch.total_width, hbox_scratch.height},
        hbox_scratch.ascent);
    float const total_extra_space
        = (std::max)(0.f, placement.size.x - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    float current_x = box.pos.x + placement.pos.x;
    for (LayoutNode* child = hbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_boxes(
            ctx,
            child,
            Box{.pos = Vec2{current_x, box.pos.y + placement.pos.y},
                .size = Vec2{child_x.min_size + extra_space, box.size.y}},
            baseline);
        current_x += child_x.min_size + extra_space;
    }
}

LayoutNodeVtable hbox_vtable
    = {hbox_measure_horizontal,
       hbox_assign_widths,
       hbox_measure_vertical,
       hbox_assign_boxes,
       hbox_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
