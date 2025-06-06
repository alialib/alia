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
gather_hbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = claim_scratch<HBoxScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x
            = gather_x_requirements(specs, scratch_arena, child_index);
        *x_requirements++ = child_x;
        hbox_scratch.total_width += child_x.min_size;
        hbox_scratch.total_growth += child_x.growth_factor;
    }
    return HorizontalRequirements{
        .min_size = hbox_scratch.total_width + hbox.margin.x * 2,
        .growth_factor = 0};
}

HorizontalRequirements
recall_hbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = peek_scratch<HBoxScratch>(scratch_arena);
    return HorizontalRequirements{
        .min_size = hbox_scratch.total_width + hbox.margin.x * 2,
        .growth_factor = 0};
}

void
assign_hbox_widths(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = use_scratch<HBoxScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    assigned_width -= hbox.margin.x * 2;
    float const total_extra_space
        = (std::max)(0.f, assigned_width - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_widths(
            specs, scratch_arena, child_x.min_size + extra_space, child_index);
    }
}

VerticalRequirements
gather_hbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = use_scratch<HBoxScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    // TODO: Stop repeating this logic everywhere.
    assigned_width -= hbox.margin.y * 2;
    float const total_extra_space
        = (std::max)(0.f, assigned_width - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y = gather_y_requirements(
            specs, scratch_arena, child_x.min_size + extra_space, child_index);
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
        .min_size = hbox_scratch.height + hbox.margin.y * 2,
        .growth_factor = 0,
        .has_baseline = hbox_scratch.has_baseline,
        .baseline_offset = hbox_scratch.ascent};
}

VerticalRequirements
recall_hbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = peek_scratch<HBoxScratch>(scratch_arena);
    return VerticalRequirements{
        .min_size = hbox_scratch.height + hbox.margin.y * 2,
        .growth_factor = 0,
        .has_baseline = hbox_scratch.has_baseline,
        .baseline_offset = hbox_scratch.ascent};
}

void
assign_hbox_boxes(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = use_scratch<HBoxScratch>(scratch_arena);
    HorizontalRequirements* x_requirements
        = reinterpret_cast<HorizontalRequirements*>(scratch_arena.allocate(
            hbox.child_count * sizeof(HorizontalRequirements),
            alignof(HorizontalRequirements)));
    box = apply_margin(box, hbox.margin);
    float const total_extra_space
        = (std::max)(0.f, box.size.x - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, hbox_scratch.total_growth);
    float current_x = box.pos.x;
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_boxes(
            specs,
            scratch_arena,
            placements,
            Box{Vec2{current_x, box.pos.y},
                Vec2{child_x.min_size + extra_space, box.size.y}},
            child_index);
        current_x += child_x.min_size + extra_space;
    }
}

} // namespace alia
