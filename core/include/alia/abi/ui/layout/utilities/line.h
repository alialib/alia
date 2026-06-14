#ifndef ALIA_ABI_UI_LAYOUT_UTILITIES_LINE_H
#define ALIA_ABI_UI_LAYOUT_UTILITIES_LINE_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/protocol.h>

ALIA_EXTERN_C_BEGIN

static inline void
alia_layout_line_reset(alia_line_requirements* line)
{
    line->height = 0;
    line->ascent = 0;
    line->descent = 0;
}

static inline void
alia_layout_line_fold_in_child(
    alia_line_requirements* line, alia_vertical_requirements const* child)
{
    line->height = alia_max(line->height, child->min_size);
    line->ascent = alia_max(line->ascent, child->ascent);
    line->descent = alia_max(line->descent, child->descent);
}

static inline void
alia_layout_line_fold_in_fragment(
    alia_line_requirements* line, alia_flow_fragment const* fragment)
{
    line->height = alia_max(line->height, fragment->height);
    line->ascent = alia_max(line->ascent, fragment->ascent);
    line->descent = alia_max(line->descent, fragment->descent);
}

static inline float
alia_layout_line_final_height(alia_line_requirements const* line)
{
    return alia_max(line->height, line->ascent + line->descent);
}

static inline void
alia_layout_line_finalize_height(alia_line_requirements* line)
{
    line->height = alia_layout_line_final_height(line);
}

static inline void
alia_layout_line_finalize_height_with_minimum(
    alia_line_requirements* line, float minimum_line_height)
{
    alia_layout_line_finalize_height(line);
    line->height = alia_max(line->height, minimum_line_height);
}

typedef struct alia_layout_line_justification_spacing
{
    float before_items;
    float between_items;
} alia_layout_line_justification_spacing;

alia_layout_line_justification_spacing
alia_layout_justify_line(
    alia_layout_flags_t flags, float extra_space, int item_count);

// Returns the justify flags for an incomplete (or intentionally broken) line.
// Justification flags that were meant to insert space between fragments are
// replaced by JUSTIFY_START.
static inline alia_layout_flags_t
alia_layout_justify_flags_for_incomplete_line(alia_layout_flags_t flags)
{
    alia_layout_flags_t justify = flags & ALIA_JUSTIFY_MASK;
    return justify > ALIA_JUSTIFY_CENTER ? ALIA_JUSTIFY_START : justify;
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_UTILITIES_LINE_H
