#pragma once

#include <cstdint>

#include <alia/flow/infinite_arena.hpp>
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
    Text,
};

struct CustomLayoutNodeVtable;

struct CustomLayoutNode
{
    CustomLayoutNodeVtable* vtable;
    void* data;
};

struct LayoutContainer
{
    LayoutNode* first_child;
    uint32_t child_count;
};

struct LayoutLeafNode
{
};

struct LayoutNode
{
    LayoutNodeType type;
    LayoutNode* next_sibling;

    Vec2 size;
    Vec2 margin;
    float growth_factor;

    union
    {
        LayoutContainer container;
        LayoutLeafNode leaf;
        CustomLayoutNode custom;
    };
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

LayoutPlacement*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space);

HorizontalRequirements
gather_x_requirements(LayoutScratchArena& scratch, LayoutNode& node);

HorizontalRequirements
recall_x_requirements(LayoutScratchArena& scratch, LayoutNode& node);

void
assign_widths(
    LayoutScratchArena& scratch, LayoutNode& node, float assigned_width);

struct VerticalRequirements
{
    float min_size;
    float growth_factor = 0.f; // 0 = fixed, >0 = wants to grow
    bool has_baseline = false;
    float baseline_offset = 0.f;
};

VerticalRequirements
gather_y_requirements(
    LayoutScratchArena& scratch, LayoutNode& node, float assigned_width);

VerticalRequirements
recall_y_requirements(LayoutScratchArena& scratch, LayoutNode& node);

struct PlacementContext
{
    LayoutScratchArena* scratch;
    LayoutPlacementArena* arena;
    LayoutPlacement** next_ptr;
};

void
assign_boxes(PlacementContext& ctx, LayoutNode& node, Box box);

struct CustomLayoutNodeVtable
{
    /*void (*gather_x_requirements)(
        LayoutNode* node, LayoutScratchArena& scratch);
    void (*recall_x_requirements)(
        LayoutNode* node, LayoutScratchArena& scratch);
    void (*assign_widths)(
        LayoutNode* node, LayoutScratchArena& scratch, float assigned_width);
    void (*gather_y_requirements)(
        LayoutNode* node, LayoutScratchArena& scratch, float assigned_width);
    void (*recall_y_requirements)(
        LayoutNode* node, LayoutScratchArena& scratch);
    void (*assign_boxes)(
        LayoutNode* node, LayoutScratchArena& scratch, Box box);*/
};

} // namespace alia
