#pragma once

#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/text.h>
#include <alia/context.h>
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
    alia_app* app, size_t body_font_index, size_t heading_font_index);

static inline alia_palette_color
demo_text_color(enum alia_palette_ramp_level level)
{
    return alia_palette_color_foundation(
        ALIA_PALETTE_FOUNDATION_RAMP_TEXT, level, 0xff);
}

// Emit text with an already-resolved font (NULL => inherit the active font).
void
demo_text(
    alia::context& ctx,
    char const* text,
    alia_resolved_font const* font,
    alia_palette_color color,
    alia::layout_flag_set flags = alia::NO_FLAGS);
