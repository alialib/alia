#ifndef ALIA_ABI_UI_LAYOUT_UTILITIES_PLACEMENT_H
#define ALIA_ABI_UI_LAYOUT_UTILITIES_PLACEMENT_H

#include <alia/abi/base/geometry/types.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/protocol.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_layout_axis_placement
{
    float offset;
    float size;
} alia_layout_axis_placement;

// CONTAINER PLACEMENT - If no alignment flags are set, the node will be
// stretched to fill its assigned region.

alia_layout_axis_placement
alia_resolve_container_x(
    alia_layout_flags_t flags, float assigned_size, float required_size);

alia_layout_axis_placement
alia_resolve_container_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent);

alia_box
alia_resolve_container_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent);

// LEAF PLACEMENT - Takes into account spacing. If no alignment flags are set,
// the node will be aligned to the start of its assigned region.

alia_layout_axis_placement
alia_resolve_leaf_x(
    alia_layout_flags_t flags,
    float assigned_size,
    float required_size,
    float spacing);

alia_layout_axis_placement
alia_resolve_leaf_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float spacing);

alia_box
alia_resolve_leaf_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent,
    alia_vec2f spacing);

float
alia_resolve_baseline(
    alia_layout_flags_t flags,
    float assigned_height,
    float ascent,
    float descent);

static inline float
alia_resolve_growth_factor(alia_layout_flags_t flags)
{
    return (flags & ALIA_GROW) ? 1.0f : 0.0f;
}

// Return `mask` if any bit of `flags & mask` is set and 0 otherwise.
static inline alia_layout_flags_t
alia_alignment_group_mask(alia_layout_flags_t flags, alia_layout_flags_t mask)
{
    return mask & (0u - (alia_layout_flags_t) ((flags & mask) != 0));
}

// Resolve caller and default alignment (including CROSS) into absolute X/Y
// for the given parent main axis. Overall priority order is: caller absolute,
// then caller CROSS, then default absolute, and finally default CROSS.
// Non-alignment flags are preserved.
static inline alia_layout_flags_t
alia_resolve_alignment_flags(
    alia_layout_flags_t flags, alia_main_axis_index main_axis)
{
    alia_layout_flags_t const xy_mask
        = ALIA_X_ALIGNMENT_MASK | ALIA_Y_ALIGNMENT_MASK;
    // Calculate the absolute mask corresponding to the cross axis.
    alia_layout_flags_t const cross_abs_mask
        = ALIA_X_ALIGNMENT_MASK << (6u - (unsigned) main_axis);

    alia_layout_flags_t const user = flags & ALIA_ALIGNMENT_MASK;
    alia_layout_flags_t const defs
        = (flags >> ALIA_DEFAULT_ALIGNMENT_BIT_OFFSET) & ALIA_ALIGNMENT_MASK;

    // Resolve absolute vs cross within the user flags.
    alia_layout_flags_t const user_xy = user & xy_mask;
    alia_layout_flags_t const user_level
        = user_xy & ~alia_alignment_group_mask(user_xy, cross_abs_mask)
        // Cross axis flags only apply if the corresponding absolute flag is
        // not set.
        | ((((user & ALIA_CROSS_ALIGNMENT_MASK) >> main_axis) & cross_abs_mask)
           & ~alia_alignment_group_mask(user_xy, cross_abs_mask));

    // Do the same for the default flags.
    alia_layout_flags_t const def_xy = defs & xy_mask;
    alia_layout_flags_t const def_level
        = def_xy
        | ((((defs & ALIA_CROSS_ALIGNMENT_MASK) >> main_axis) & cross_abs_mask)
           & ~alia_alignment_group_mask(def_xy, cross_abs_mask));

    // Now resolve the user-level flags against the default-level flags.
    alia_layout_flags_t const valid_user_level_mask
        = alia_alignment_group_mask(user_level, ALIA_X_ALIGNMENT_MASK)
        | alia_alignment_group_mask(user_level, ALIA_Y_ALIGNMENT_MASK);
    return (flags & ~(ALIA_ALIGNMENT_MASK | ALIA_DEFAULT_ALIGNMENT_MASK))
         | user_level | (def_level & ~valid_user_level_mask);
}

// Returns true when a node participates in baseline alignment with its parent
// along the axis measured by measure_vertical.
static inline bool
alia_participates_in_parent_baseline_alignment(
    alia_layout_flags_t flags, alia_main_axis_index main_axis)
{
    return (alia_resolve_alignment_flags(flags, main_axis)
            & ALIA_Y_ALIGNMENT_MASK)
        == ALIA_BASELINE_Y;
}

// `ascent` and `descent` should be 0 for children that don't participate in
// baseline alignment. This function is used to mask those values (when
// appropriate) when reporting vertical requirements to a parent.
static inline alia_vertical_requirements
alia_mask_reported_vertical_requirements(
    alia_layout_flags_t flags,
    alia_main_axis_index main_axis,
    alia_vertical_requirements requirements)
{
    if (!alia_participates_in_parent_baseline_alignment(flags, main_axis))
    {
        requirements.ascent = 0.f;
        requirements.descent = 0.f;
    }
    return requirements;
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_UTILITIES_PLACEMENT_H
