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
gather_flow_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& flow)
{
    auto& flow_scratch = claim_scratch<FlowScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    for (LayoutIndex child_index = flow.first_child; child_index != 0;
         child_index = nodes[child_index].next_sibling)
    {
        auto const child_x
            = gather_x_requirements(nodes, scratch_arena, child_index);
        *x_requirements++ = child_x;
        flow_scratch.max_child_width
            = (std::max)(flow_scratch.max_child_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = flow_scratch.max_child_width + flow.margin.x * 2,
        .growth_factor = 0};
}

HorizontalRequirements
recall_flow_x_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& flow)
{
    auto& flow_scratch = peek_scratch<FlowScratch>(scratch_arena);
    return HorizontalRequirements{
        .min_size = flow_scratch.max_child_width + flow.margin.x * 2,
        .growth_factor = 0};
}

void
assign_flow_widths(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& flow)
{
    auto& flow_scratch = use_scratch<FlowScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    assigned_width -= flow.margin.x * 2;
    for (LayoutIndex child_index = flow.first_child; child_index != 0;
         child_index = nodes[child_index].next_sibling)
    {
        auto const child_x = *x_requirements++;
        assign_widths(nodes, scratch_arena, child_x.min_size, child_index);
    }
}

VerticalRequirements
gather_flow_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutNode const& flow)
{
    auto& flow_scratch = use_scratch<FlowScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    float row_height = 0, total_height = 0, row_width = 0;
    for (LayoutIndex child_index = flow.first_child; child_index != 0;
         child_index = nodes[child_index].next_sibling)
    {
        auto const child_x = *x_requirements++;
        row_width += child_x.min_size;
        if (row_width > assigned_width)
        {
            total_height += row_height;
            row_height = 0;
            row_width = child_x.min_size;
        }
        auto const child_y = gather_y_requirements(
            nodes, scratch_arena, child_x.min_size, child_index);
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
    flow_scratch.height = (std::max)(
        flow_scratch.height, flow_scratch.ascent + flow_scratch.descent);
    return VerticalRequirements{
        .min_size = flow_scratch.height + flow.margin.y * 2,
        .growth_factor = 0,
        .has_baseline = flow_scratch.has_baseline,
        .baseline_offset = flow_scratch.ascent};
}

VerticalRequirements
recall_flow_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutNode const& flow)
{
    auto& flow_scratch = peek_scratch<FlowScratch>(scratch_arena);
    return VerticalRequirements{
        .min_size = flow_scratch.height + flow.margin.y * 2,
        .growth_factor = 0,
        .has_baseline = flow_scratch.has_baseline,
        .baseline_offset = flow_scratch.ascent};
}

void
assign_flow_boxes(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutNode const& flow)
{
    auto& flow_scratch = use_scratch<FlowScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    VerticalRequirements* y_requirements
        = reinterpret_cast<VerticalRequirements*>(scratch_arena.allocate(
            flow.child_count * sizeof(VerticalRequirements),
            alignof(VerticalRequirements)));
    box = apply_margin(box, flow.margin);
    float current_x = box.pos.x, current_y = box.pos.y;
    float row_height = 0;
    for (LayoutIndex child_index = flow.first_child; child_index != 0;
         child_index = nodes[child_index].next_sibling)
    {
        auto const child_x = *x_requirements++;
        auto const child_y = *y_requirements++;
        if (current_x + child_x.min_size > box.size.x)
        {
            current_y += row_height;
            row_height = 0;
            current_x = box.pos.x;
        }
        row_height = (std::max)(row_height, child_y.min_size);
        assign_boxes(
            nodes,
            scratch_arena,
            placements,
            Box{Vec2{current_x, current_y},
                Vec2{child_x.min_size, row_height}},
            child_index);
        current_x += child_x.min_size;
    }
}

} // namespace alia
