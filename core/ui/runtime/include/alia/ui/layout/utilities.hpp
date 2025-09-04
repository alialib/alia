#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

// ALIGNMENT UTILITIES

struct layout_axis_placement
{
    float offset;
    float size;
};

layout_axis_placement
resolve_horizontal_assignment(
    layout_flag_set flags, float assigned_size, float required_size);

layout_axis_placement
resolve_vertical_assignment(
    layout_flag_set flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent);

box
resolve_assignment(
    layout_flag_set flags,
    vec2 assigned_size,
    float baseline,
    vec2 required_size,
    float ascent);

layout_axis_placement
resolve_padded_horizontal_assignment(
    layout_flag_set flags,
    float assigned_size,
    float required_size,
    float padding);

layout_axis_placement
resolve_padded_vertical_assignment(
    layout_flag_set flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float padding);

box
resolve_padded_assignment(
    layout_flag_set flags,
    vec2 assigned_size,
    float baseline,
    vec2 required_size,
    float ascent,
    float padding);

float
assign_baseline(
    layout_flag_set flags, float assigned_height, float ascent, float descent);

inline float
resolve_growth_factor(layout_flag_set flags)
{
    return flags & GROW ? 1.0f : 0.0f;
}

inline layout_flag_set
adjust_flags_for_main_axis(layout_flag_set flags, main_axis_index main_axis)
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
claim_scratch(infinite_arena& arena)
{
    return *arena_new<T>(arena);
}

// Use already-claimed scratch space from the arena - This is called by the
// node implementation during subsequent steps of the layout process to re-use
// the scratch space that was claimed and initialized during the horizontal
// measurement step.
template<class T>
T&
use_scratch(infinite_arena& arena)
{
    return *arena_alloc<T>(arena);
}

// DEFAULT NODE IMPLEMENTATIONS

wrapping_requirements
default_measure_wrapped_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float current_x_offset,
    float line_width);

} // namespace alia
