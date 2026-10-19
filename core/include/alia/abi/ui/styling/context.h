#ifndef ALIA_ABI_UI_STYLING_CONTEXT_H
#define ALIA_ABI_UI_STYLING_CONTEXT_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/palette.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_resolved_font alia_resolved_font;

// font and color for text
typedef struct alia_text_style
{
    // resolved font
    alia_resolved_font const* font;
    // text color
    alia_srgba8 color;
} alia_text_style;

// thickness and outset of the focus indicator
typedef struct alia_focus_ring
{
    // focus indicator thickness, logical px
    float thickness;
    // focus indicator outset beyond the control bounds, logical px
    float outset;
} alia_focus_ring;

// active focus indicator
typedef struct alia_focus
{
    // focus-indicator color
    alia_palette_color color;
    // when false, draw focus only while keyboard interaction is set
    bool show_on_pointer;
    // focus indicator thickness and outset
    alia_focus_ring ring;
} alia_focus;

// `alia_style_context` is the pass-local style state for a UI traversal.
typedef struct alia_style_context
{
    // mutable active styles for this pass - These are initialized to copies of
    // the system defaults when the context is created.
    void* active_styles;
    // active text font and color - `font` is NULL until one is pushed.
    alia_text_style text;
    // sibling gap, logical px
    float spacing;
    // active focus indicator
    alia_focus focus;
} alia_style_context;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_STYLING_CONTEXT_H */
