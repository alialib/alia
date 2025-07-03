#include <alia/ui/layout/flow.hpp>
#include <alia/ui/layout/scratch.hpp>

namespace alia {

struct FlowScratch
{
    float max_child_width = 0;
    bool has_baseline = false;
    float height = 0, ascent = 0, descent = 0;
};

HorizontalRequirements
gather_flow_x_requirements(LayoutScratchArena* scratch, LayoutNode* node)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = claim_scratch<FlowScratch>(*scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch->allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = gather_x_requirements(scratch, child);
        *x_requirements++ = child_x;
        flow_scratch.max_child_width
            = (std::max)(flow_scratch.max_child_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = flow_scratch.max_child_width, .growth_factor = 0};
}

void
assign_flow_widths(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = claim_scratch<FlowScratch>(*scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch->allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        assign_widths(scratch, child, child_x.min_size);
    }
}

VerticalRequirements
gather_flow_y_requirements(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = claim_scratch<FlowScratch>(*scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch->allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    float row_height = 0, total_height = 0, row_width = 0;
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        row_width += child_x.min_size;
        if (row_width > assigned_width)
        {
            total_height += row_height;
            row_height = 0;
            row_width = child_x.min_size;
        }
        auto const child_y
            = gather_y_requirements(scratch, child, child_x.min_size);
        *y_requirements++ = child_y;
        row_height = (std::max)(row_height, child_y.min_size);
        // TODO: Add baseline support.
        // if (child_y.has_baseline)
        // {
        //     flow_scratch.ascent
        //         = (std::max)(flow_scratch.ascent, child_y.baseline_offset);
        //     flow_scratch.descent = (std::max)(
        //         flow_scratch.descent,
        //         child_y.min_size - child_y.baseline_offset);
        //     flow_scratch.has_baseline = true;
        // }
    }
    total_height += row_height;
    flow_scratch.height
        = (std::max)(total_height, flow_scratch.ascent + flow_scratch.descent);
    return VerticalRequirements{
        .min_size = flow_scratch.height,
        .growth_factor = 0,
        .has_baseline = flow_scratch.has_baseline,
        .baseline_offset = flow_scratch.ascent};
}

void
assign_flow_boxes(PlacementContext* ctx, LayoutNode* node, Box box)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = claim_scratch<FlowScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(ctx->scratch->allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(ctx->scratch->allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    float current_x = 0, current_y = box.pos.y;
    float row_height = 0;
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        auto const child_y = *y_requirements++;
        if (current_x + child_x.min_size > box.size.x)
        {
            current_y += row_height;
            row_height = 0;
            current_x = 0;
        }
        row_height = (std::max)(row_height, child_y.min_size);
        assign_boxes(
            ctx,
            child,
            Box{Vec2{box.pos.x + current_x, current_y},
                Vec2{child_x.min_size, row_height}});
        current_x += child_x.min_size;
    }
}

LayoutNodeVtable flow_vtable = {
    gather_flow_x_requirements,
    assign_flow_widths,
    gather_flow_y_requirements,
    assign_flow_boxes,
};

} // namespace alia
