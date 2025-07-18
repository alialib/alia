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
vbox_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& vbox = *reinterpret_cast<VBoxLayoutNode*>(node);
    auto& vbox_scratch = claim_scratch<VBoxScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(ctx->scratch->allocate(
            vbox.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = vbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        vbox_scratch.max_width
            = (std::max)(vbox_scratch.max_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = vbox_scratch.max_width, .growth_factor = 0};
}

void
vbox_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& vbox = *reinterpret_cast<VBoxLayoutNode*>(node);
    auto& vbox_scratch = use_scratch<VBoxScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(ctx->scratch->allocate(
            vbox.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = vbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        assign_widths(ctx, child, assigned_width);
    }
}

VerticalRequirements
vbox_measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& vbox = *reinterpret_cast<VBoxLayoutNode*>(node);
    auto& vbox_scratch = use_scratch<VBoxScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(ctx->scratch->allocate(
            vbox.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = vbox.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = measure_vertical(ctx, child, assigned_width);
        *y_requirements++ = child_y;
        vbox_scratch.total_height += child_y.min_size;
        vbox_scratch.total_growth += child_y.growth_factor;
        // TODO: Handle baseline.
    }
    return VerticalRequirements{
        .min_size = vbox_scratch.total_height, .growth_factor = 0};
}

void
vbox_assign_boxes(
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
    = {vbox_measure_horizontal,
       vbox_assign_widths,
       vbox_measure_vertical,
       vbox_assign_boxes,
       vbox_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
