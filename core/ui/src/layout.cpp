#include <alia/ui/layout.hpp>

#include <algorithm>

namespace alia {

void
resolve_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Vec2 available_space)
{
    scratch.reset();
    gather_x_requirements(specs, scratch, 1);
    scratch.reset();
    assign_x_layout(specs, scratch, available_space.x, 1);
    scratch.reset();
    gather_y_requirements(specs, scratch, available_space.y, 1);
    scratch.reset();
    assign_y_layout(
        specs, scratch, placements, Box{Vec2{0, 0}, available_space}, 1);
    scratch.reset();
}

template<class T>
T&
claim_scratch(LayoutScratchArena& arena)
{
    static_assert(std::is_trivially_destructible_v<T>);
    void* raw = arena.allocate(sizeof(T), alignof(T));
    return *std::construct_at(reinterpret_cast<T*>(raw));
}

template<class T>
T&
use_scratch(LayoutScratchArena& arena)
{
    void* raw = arena.allocate(sizeof(T), alignof(T));
    return *reinterpret_cast<T*>(raw);
}

template<class T>
T&
peek_scratch(LayoutScratchArena& arena)
{
    return *reinterpret_cast<T*>(arena.peek());
}

// HBOX FUNCTIONS

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
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x
            = gather_x_requirements(specs, scratch_arena, child_index);
        hbox_scratch.total_width += child_x.min_size;
        hbox_scratch.total_growth += child_x.growth_factor;
    }
    return HorizontalRequirements{
        .min_size = hbox_scratch.total_width + hbox.margin.x * 2,
        .growth_factor = hbox_scratch.total_growth};
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
        .growth_factor = hbox_scratch.total_growth};
}

void
assign_hbox_x_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    float assigned_width,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = use_scratch<HBoxScratch>(scratch_arena);
    assigned_width -= hbox.margin.x * 2;
    float const total_extra_space
        = (std::max)(0.f, assigned_width - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max)(0.00001f, hbox_scratch.total_growth);
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x
            = recall_x_requirements(specs, scratch_arena, child_index);
        float const extra_space
            = total_extra_space * child_x.growth_factor / total_growth;
        assign_x_layout(
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
    // TODO: Stop repeating this logic everywhere.
    assigned_width -= hbox.margin.y * 2;
    float const total_extra_space
        = (std::max)(0.f, assigned_width - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max)(0.00001f, hbox_scratch.total_growth);
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x
            = recall_x_requirements(specs, scratch_arena, child_index);
        float const extra_space
            = total_extra_space * child_x.growth_factor / total_growth;
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
        .growth_factor = hbox_scratch.total_growth,
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
        .growth_factor = hbox_scratch.total_growth,
        .has_baseline = hbox_scratch.has_baseline,
        .baseline_offset = hbox_scratch.ascent};
}

Box
apply_margin(Box box, Vec2 margin)
{
    return {.pos = box.pos + margin, .size = box.size - margin * 2};
}

void
assign_hbox_y_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch_arena,
    LayoutPlacement* placements,
    Box box,
    LayoutSpec const& hbox)
{
    auto& hbox_scratch = use_scratch<HBoxScratch>(scratch_arena);
    box = apply_margin(box, hbox.margin);
    float const total_extra_space
        = (std::max)(0.f, box.size.x - hbox_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max)(0.00001f, hbox_scratch.total_growth);
    float current_x = box.pos.x;
    for (LayoutIndex child_index = hbox.first_child; child_index != 0;
         child_index = specs[child_index].next_sibling)
    {
        auto const child_x
            = recall_x_requirements(specs, scratch_arena, child_index);
        float const extra_space
            = total_extra_space * child_x.growth_factor / total_growth;
        assign_y_layout(
            specs,
            scratch_arena,
            placements,
            Box{Vec2{current_x, box.pos.y},
                Vec2{child_x.min_size + extra_space, box.size.y}},
            child_index);
        current_x += child_x.min_size + extra_space;
    }
}

// VBOX FUNCTIONS

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
        .growth_factor = hbox_scratch.total_growth};
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
        .growth_factor = vbox_scratch.total_growth};
}

void
assign_vbox_x_layout(
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
        assign_x_layout(specs, scratch_arena, assigned_width, child_index);
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
        .growth_factor = vbox_scratch.total_growth};
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
        .growth_factor = vbox_scratch.total_growth};
}

void
assign_vbox_y_layout(
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
        assign_y_layout(
            specs,
            scratch_arena,
            placements,
            Box{Vec2{0, current_y},
                Vec2{box.size.x, child_y.min_size + extra_space}},
            child_index);
        current_y += child_y.min_size + extra_space;
    }
}

// TOP-LEVEL LAYOUT FUNCTIONS

HorizontalRequirements
gather_x_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_x_requirements(specs, scratch, spec);
        case LayoutNodeType::VBox:
            return gather_vbox_x_requirements(specs, scratch, spec);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = spec.size.x + spec.margin.x * 2,
                .growth_factor = spec.growth_factor};
    }
}

HorizontalRequirements
recall_x_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_x_requirements(specs, scratch, spec);
        case LayoutNodeType::VBox:
            return recall_vbox_x_requirements(specs, scratch, spec);
        default:
        case LayoutNodeType::Leaf:
            return HorizontalRequirements{
                .min_size = spec.size.x + spec.margin.x * 2,
                .growth_factor = spec.growth_factor};
    }
}

void
assign_x_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_x_layout(specs, scratch, assigned_width, spec);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_x_layout(specs, scratch, assigned_width, spec);
            break;
        default:
        case LayoutNodeType::Leaf:
            break;
    }
}

VerticalRequirements
gather_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return gather_hbox_y_requirements(
                specs, scratch, assigned_width, spec);
        case LayoutNodeType::VBox:
            return gather_vbox_y_requirements(
                specs, scratch, assigned_width, spec);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = spec.size.y + spec.margin.y * 2,
                .growth_factor = spec.growth_factor};
    }
}

VerticalRequirements
recall_y_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            return recall_hbox_y_requirements(specs, scratch, spec);
        case LayoutNodeType::VBox:
            return recall_vbox_y_requirements(specs, scratch, spec);
        default:
        case LayoutNodeType::Leaf:
            return VerticalRequirements{
                .min_size = spec.size.y + spec.margin.y * 2,
                .growth_factor = spec.growth_factor};
    }
}

void
assign_y_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Box box,
    LayoutIndex index)
{
    auto& spec = specs[index];
    switch (spec.type)
    {
        case LayoutNodeType::HBox:
            assign_hbox_y_layout(specs, scratch, placements, box, spec);
            break;
        case LayoutNodeType::VBox:
            assign_vbox_y_layout(specs, scratch, placements, box, spec);
            break;
        default:
        case LayoutNodeType::Leaf:
            placements[index].position = box.pos + spec.margin;
            placements[index].size = spec.size - spec.margin * 2;
            break;
    }
}

} // namespace alia
