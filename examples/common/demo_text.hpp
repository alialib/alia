#pragma once

#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/styling/palette_generation.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/context.hpp>
#include <alia/shell/app.h>
#include <alia/ui/layout/flags.hpp>

// SHARED DEMO HELPERS - pre-resolved stock typography plus a thin text emitter
// Call `demo_setup_fonts` after `alia_app_setup_stock_text`.

struct demo_fonts
{
    alia_resolved_font body_14;
    alia_resolved_font heading_14;
    alia_resolved_font heading_18;
};

demo_fonts const&
demo_get_fonts();

// Resolve Roboto (stock typography) into `demo_get_fonts()`.
void
demo_setup_fonts(alia_app* app);

// Re-resolve `demo_get_fonts()` from MSDF font indices (for apps that switch
// among faces in a richer atlas).
void
demo_set_typefaces(
    alia_app* app,
    size_t body_font_index,
    size_t heading_font_index,
    float scale);

// relative ink weight against the TEXT role (theme-relative strengthen/weaken)
enum demo_ink_weight
{
    DEMO_INK_MUTED = 0,
    DEMO_INK_BODY,
    DEMO_INK_EMPHASIS,
    DEMO_INK_STRONG,
};

// Resolve demo ink against the active palette.
alia_srgba8
demo_ink(alia::ui_context ctx, demo_ink_weight weight);

// page / raised / accent-wash fills for demo panels (opaque sRGB)
alia_srgb8
demo_surface(alia::ui_context ctx);
alia_srgb8
demo_raised(alia::ui_context ctx);
alia_srgb8
demo_accent_wash(alia::ui_context ctx);
alia_srgba8
demo_on_accent_wash(alia::ui_context ctx);

// Emit text with an already-resolved font (NULL => inherit the active font).
void
demo_text(
    alia::ui_context ctx,
    char const* text,
    alia_resolved_font const* font,
    alia_srgba8 color,
    alia::layout_flag_set flags = alia::NO_FLAGS);

static inline void
demo_text(
    alia::ui_context ctx,
    char const* text,
    alia_resolved_font const* font,
    demo_ink_weight weight,
    alia::layout_flag_set flags = alia::NO_FLAGS)
{
    demo_text(ctx, text, font, demo_ink(ctx, weight), flags);
}
