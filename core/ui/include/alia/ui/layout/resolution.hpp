#pragma once

#include <cstdint>

#include <alia/foundation/infinite_arena.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/api.hpp>

namespace alia {

enum class LayoutNodeType
{
    Leaf,
    HBox,
    VBox,
    Stack,
    Flow,
};

struct LayoutNode
{
    LayoutNodeType type;

    //  TODO: Unionize this...

    Vec2 size;
    Vec2 margin;
    float growth_factor = 0.f; // 0: fixed, >0: wants to grow

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
    float growth_factor = 0.f; // 0: fixed, >0: wants to grow
};

void
resolve_layout(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Vec2 available_space);

HorizontalRequirements
gather_x_requirements(
    LayoutNode const* nodes, LayoutScratchArena& scratch, LayoutIndex index);

HorizontalRequirements
recall_x_requirements(
    LayoutNode const* nodes, LayoutScratchArena& scratch, LayoutIndex index);

void
assign_widths(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index);

struct VerticalRequirements
{
    float min_size;
    float growth_factor = 0.f; // 0 = fixed, >0 = wants to grow
    bool has_baseline = false;
    float baseline_offset = 0.f;
};

VerticalRequirements
gather_y_requirements(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    float assigned_width,
    LayoutIndex index);

VerticalRequirements
recall_y_requirements(
    LayoutNode const* nodes, LayoutScratchArena& scratch, LayoutIndex index);

void
assign_boxes(
    LayoutNode const* nodes,
    LayoutScratchArena& scratch,
    LayoutPlacement* placements,
    Box box,
    LayoutIndex index);

} // namespace alia
