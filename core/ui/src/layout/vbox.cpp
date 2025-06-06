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
gather_vbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& vbox)
{
    auto& hbox_scratch = claim_scratch<VBoxScratch>(scratch_arena);
    for (LayoutIndex child_index = vbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x
            = gather_x_requirements(specs, scratch_arena, child_index);
        hbox_scratch.max_width
            = (std::max)(hbox_scratch.max_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = hbox_scratch.max_width + vbox.margin.x * 2,
        .growth_factor = 0};
}

HorizontalRequirements
recall_vbox_x_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& vbox)
{
    auto& vbox_scratch = peek_scratch<VBoxScratch>(scratch_arena);
    return HorizontalRequirements{
        .min_size = vbox_scratch.max_width + vbox.margin.x * 2,
        .growth_factor = 0};
}

void
assign_vbox_widths(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& vbox)
{
    auto& vbox_scratch = use_scratch<VBoxScratch>(scratch_arena);
    assigned_width -= vbox.margin.x * 2;
    for (LayoutIndex child_index = vbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        assign_widths(specs, scratch_arena, assigned_width, child_index);
    }
}

VerticalRequirements
gather_vbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& vbox)
{
    auto& vbox_scratch = use_scratch<VBoxScratch>(scratch_arena);
    assigned_width -= vbox.margin.y * 2;
    for (LayoutIndex child_index = vbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_y = gather_y_requirements(
            specs, scratch_arena, assigned_width, child_index);
        vbox_scratch.total_height += child_y.min_size;
        vbox_scratch.total_growth += child_y.growth_factor;
        // TODO: Handle baseline.
    }
    return VerticalRequirements{
        .min_size = vbox_scratch.total_height + vbox.margin.y * 2,
        .growth_factor = 0};
}

VerticalRequirements
recall_vbox_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutSpec const& vbox)
{
    auto& vbox_scratch = peek_scratch<VBoxScratch>(scratch_arena);
    return VerticalRequirements{
        .min_size = vbox_scratch.total_height + vbox.margin.y * 2,
        .growth_factor = 0};
}

void
assign_vbox_boxes(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutSpec const& vbox)
{
    auto& vbox_scratch = use_scratch<VBoxScratch>(scratch_arena);
    box = apply_margin(box, vbox.margin);
    float const total_extra_space
        = (std::max)(0.f, box.size.y - vbox_scratch.total_height);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max)(0.00001f, vbox_scratch.total_growth);
    float current_y = box.pos.y;
    for (LayoutIndex child_index = vbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_y
            = recall_y_requirements(specs, scratch_arena, child_index);
        float const extra_space
            = total_extra_space * child_y.growth_factor / total_growth;
        assign_boxes(
            specs,
            scratch_arena,
            placements,
            Box{Vec2{0, current_y},
                Vec2{box.size.x, child_y.min_size + extra_space}},
            child_index);
        current_y += child_y.min_size + extra_space;
    }
}

} // namespace alia
