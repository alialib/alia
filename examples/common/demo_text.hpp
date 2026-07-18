#pragma once

#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/text.h>
#include <alia/context.h>
#include <alia/shell/app.h>
#include <alia/ui/layout/flags.hpp>

// Small shared demo helpers: pre-resolved stock fonts + a thin text emitter.
// Fonts are filled once at setup; call sites pass pointers (no per-call
// resolve).

struct demo_fonts
{
    alia_resolved_font body_14;
    alia_resolved_font heading_14;
    alia_resolved_font heading_18;
};

demo_fonts const&
demo_get_fonts();

// Resolve the stock typefaces into `demo_get_fonts()` after
// `alia_app_setup_stock_text`.
void
demo_setup_fonts(alia_app* app);

static inline alia_palette_color
demo_text_color(enum alia_palette_ramp_level level)
{
    return alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_TEXT, level),
        0xff);
}

// Emit text with an already-resolved font (NULL => inherit the active font).
void
demo_text(
    alia::context& ctx,
    char const* text,
    alia_resolved_font const* font,
    alia_palette_color color,
    alia::layout_flag_set flags = alia::NO_FLAGS);
