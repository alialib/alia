#pragma once

#include <cstdint>

#include <alia/flow/infinite_arena.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/api.hpp>

namespace alia {

struct LayoutNodeVtable;

struct LayoutNode
{
    LayoutNodeVtable* vtable;
    LayoutNode* next_sibling;

    // TODO: Should this really be here?
    float growth_factor;
};

struct LayoutContainer
{
    LayoutNode base;

    LayoutNode* first_child;
    uint32_t child_count;
};

using LayoutSpecArena = HeterogeneousInfiniteArena;

using LayoutScratchArena = UniformlyAlignedInfiniteArena<16>;

using LayoutPlacementArena = HeterogeneousInfiniteArena;

struct LayoutPlacement
{
    Vec2 position;
    Vec2 size;
    LayoutPlacement* next;
};

struct HorizontalRequirements
{
    float min_size;
    float growth_factor = 0.f; // 0: fixed, >0: wants to grow
};

struct VerticalRequirements
{
    float min_size;
    float growth_factor = 0.f; // 0 = fixed, >0 = wants to grow
    bool has_baseline = false;
    float baseline_offset = 0.f;
};

struct PlacementContext
{
    LayoutScratchArena* scratch;
    LayoutPlacementArena* arena;
    LayoutPlacement** next_ptr;
};

struct LayoutNodeVtable
{
    HorizontalRequirements (*gather_x_requirements)(
        LayoutScratchArena* scratch, LayoutNode* node);
    void (*assign_widths)(
        LayoutScratchArena* scratch, LayoutNode* node, float assigned_width);
    VerticalRequirements (*gather_y_requirements)(
        LayoutScratchArena* scratch, LayoutNode* node, float assigned_width);
    void (*assign_boxes)(PlacementContext* ctx, LayoutNode* node, Box box);
};

inline HorizontalRequirements
gather_x_requirements(LayoutScratchArena* scratch, LayoutNode* node)
{
    return node->vtable->gather_x_requirements(scratch, node);
}

inline void
assign_widths(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    return node->vtable->assign_widths(scratch, node, assigned_width);
}

inline VerticalRequirements
gather_y_requirements(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    return node->vtable->gather_y_requirements(scratch, node, assigned_width);
}

inline void
assign_boxes(PlacementContext* ctx, LayoutNode* node, Box box)
{
    return node->vtable->assign_boxes(ctx, node, box);
}

LayoutPlacement*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space);

} // namespace alia
