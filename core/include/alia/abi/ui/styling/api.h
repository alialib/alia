#ifndef ALIA_ABI_UI_STYLING_API_H
#define ALIA_ABI_UI_STYLING_API_H

#include <alia/abi/ui/context.h>

ALIA_EXTERN_C_BEGIN

// Make `font` the active font until the matching pop.
void
alia_font_push(alia_ui_context* ctx, alia_resolved_font const* font);

// Restore the font from the matching push.
void
alia_font_pop(alia_ui_context* ctx);

// Return the active font. Asserts if none has been pushed.
static inline alia_resolved_font const*
alia_font_active(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx && alia_ctx_style(ctx)->text.font);
    return alia_ctx_style(ctx)->text.font;
}

// Make `color` the active text color until the matching pop.
void
alia_text_color_push(alia_ui_context* ctx, alia_srgba8 color);

// Restore the text color from the matching push.
void
alia_text_color_pop(alia_ui_context* ctx);

// Return the active text color.
static inline alia_srgba8
alia_text_color_active(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    return alia_ctx_style(ctx)->text.color;
}

// Make `style` the active text style until the matching pop.
void
alia_text_style_push(alia_ui_context* ctx, alia_text_style style);

// Restore the text style from the matching push.
void
alia_text_style_pop(alia_ui_context* ctx);

// Return the active text style.
static inline alia_text_style
alia_text_style_active(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    return alia_ctx_style(ctx)->text;
}

// Make `spacing` the active spacing until the matching pop.
void
alia_spacing_push(alia_ui_context* ctx, float spacing);

// Restore the spacing from the matching push.
void
alia_spacing_pop(alia_ui_context* ctx);

// Return the active spacing, in logical px.
static inline float
alia_spacing_active(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    return alia_ctx_style(ctx)->spacing;
}

// Make `color` the active focus color until the matching pop.
void
alia_focus_color_push(alia_ui_context* ctx, alia_palette_color color);

// Restore the focus color from the matching pop.
void
alia_focus_color_pop(alia_ui_context* ctx);

// Return the active focus color.
static inline alia_palette_color
alia_focus_color_active(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    return alia_ctx_style(ctx)->focus.color;
}

// Make `show` the active pointer-focus policy until the matching pop.
void
alia_focus_show_on_pointer_push(alia_ui_context* ctx, bool show);

// Restore the pointer-focus policy from the matching push.
void
alia_focus_show_on_pointer_pop(alia_ui_context* ctx);

// Return the active pointer-focus policy.
static inline bool
alia_focus_show_on_pointer_active(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    return alia_ctx_style(ctx)->focus.show_on_pointer;
}

// Make `ring` the active focus ring until the matching pop.
void
alia_focus_ring_push(alia_ui_context* ctx, alia_focus_ring ring);

// Restore the focus ring from the matching pop.
void
alia_focus_ring_pop(alia_ui_context* ctx);

// Return the active focus ring.
static inline alia_focus_ring
alia_focus_ring_active(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    return alia_ctx_style(ctx)->focus.ring;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_STYLING_API_H */
