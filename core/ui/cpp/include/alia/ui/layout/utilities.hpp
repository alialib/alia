#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

// ALIGNMENT UTILITIES

struct LayoutAxisPlacement
{
    float offset;
    float size;
};

LayoutAxisPlacement
resolve_horizontal_assignment(
    LayoutFlagSet flags, float assigned_size, float required_size);

LayoutAxisPlacement
resolve_vertical_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent);

Box
resolve_assignment(
    LayoutFlagSet flags,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent);

LayoutAxisPlacement
resolve_padded_horizontal_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float required_size,
    float padding);

LayoutAxisPlacement
resolve_padded_vertical_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float padding);

Box
resolve_padded_assignment(
    LayoutFlagSet flags,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent,
    float padding);

inline float
resolve_growth_factor(LayoutFlagSet flags)
{
    return flags & GROW ? 1.0f : 0.0f;
}

inline LayoutFlagSet
adjust_flags_for_main_axis(LayoutFlagSet flags, MainAxisIndex main_axis)
{
    flags.code |= flags.code >> main_axis;
    return flags;
}

// SCRATCH ARENA UTILITIES

// Claim scratch space from the arena - This is called by the node
// implementation during the horizontal measurement step of the layout process.
// Since that's the first step, the scratch space is assumed to be unused at
// that point, so this will invoke the default constructor for T.
// Note that no destructor is ever called, so T must be trivially destructible.
template<class T>
T&
claim_scratch(InfiniteArena& arena)
{
    return *arena_new<T>(arena);
}

// Use already-claimed scratch space from the arena - This is called by the
// node implementation during subsequent steps of the layout process to re-use
// the scratch space that was claimed and initialized during the horizontal
// measurement step.
template<class T>
T&
use_scratch(InfiniteArena& arena)
{
    return *arena_alloc<T>(arena);
}

// DEFAULT NODE IMPLEMENTATIONS

WrappingRequirements
default_measure_wrapped_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float current_x_offset,
    float line_width);

} // namespace alia
