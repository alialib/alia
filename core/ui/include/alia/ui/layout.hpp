#pragma once

#include <alia/foundation/infinite_arena.hpp>
#include <alia/ui/geometry.hpp>

#include <cstdint>

namespace alia {

using LayoutIndex = std::uint32_t;

enum class LayoutNodeType
{
    Leaf,
    HBox,
    VBox,
    Stack,
    Flow,
};

struct LayoutSpec
{
    LayoutNodeType type;
    //  TODO: Unionize this...
    Vec2 size;
    Vec2 margin;
    float growth_factor = 0.f; // 0 = fixed, >0 = wants to grow
    LayoutIndex first_child;
    uint32_t child_count = 0;
    LayoutIndex next_sibling;
};

using LayoutScratchArena = UniformlyAlignedInfiniteArena<16>;

struct LayoutPlacement
{
    Vec2 position;
    Vec2 size;
};

struct HorizontalRequirements
{
    float min_size;
    float growth_factor = 0.f; // 0 = fixed, >0 = wants to grow
};

struct VerticalRequirements
{
    float min_size;
    float growth_factor = 0.f; // 0 = fixed, >0 = wants to grow
    bool has_baseline = false;
    float baseline_offset = 0.f;
};

void
resolve_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Vec2 available_space);

HorizontalRequirements
gather_x_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index);

HorizontalRequirements
recall_x_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index);

void
assign_x_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index);

VerticalRequirements
gather_y_requirements(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index);

VerticalRequirements
recall_y_requirements(
    LayoutSpec const* specs, LayoutScratchArena& scratch, LayoutIndex index);

void
assign_y_layout(
    LayoutSpec const* specs,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Box box,
    LayoutIndex index);

} // namespace alia
