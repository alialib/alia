#ifndef ALIA_UI_PALETTE_H
#define ALIA_UI_PALETTE_H

#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>
#include <stddef.h> // offsetof

ALIA_EXTERN_C_BEGIN

// UI lightness mode (light or dark surface)
enum alia_ui_lightness_mode
{
    // dark text on light background
    ALIA_UI_LIGHT_MODE = 0,
    // light text on dark background
    ALIA_UI_DARK_MODE = 1
};

// semantic color set for a single role (solid, subtle, outline, etc.)
typedef struct alia_swatch
{
    // solid fill
    alia_srgb8 solid;
    // content drawn on `solid`
    alia_srgb8 on_solid;

    // soft / wash fill
    alia_srgb8 subtle;
    // content drawn on `subtle`
    alia_srgb8 on_subtle;

    // border / stroke
    alia_srgb8 outline;
    // emphasized text or icon in this hue
    alia_srgb8 text;
} alia_swatch;

// nine-step lightness ramp around a foundation base
typedef struct alia_foundation_ramp
{
    alia_srgb8 weaker_4;
    alia_srgb8 weaker_3;
    alia_srgb8 weaker_2;
    alia_srgb8 weaker_1;
    alia_srgb8 base;
    alia_srgb8 stronger_1;
    alia_srgb8 stronger_2;
    alia_srgb8 stronger_3;
    alia_srgb8 stronger_4;
} alia_foundation_ramp;

// foundation ramps for background, structure, and text
typedef struct alia_foundation
{
    alia_foundation_ramp background;
    alia_foundation_ramp structural;
    alia_foundation_ramp text;
} alia_foundation;

// named hue swatches (red, orange, ...)
typedef struct alia_literal_palette
{
    alia_swatch red;
    alia_swatch orange;
    alia_swatch amber;
    alia_swatch yellow;
    alia_swatch lime;
    alia_swatch green;
    alia_swatch teal;
    alia_swatch cyan;
    alia_swatch blue;
    alia_swatch indigo;
    alia_swatch purple;
    alia_swatch pink;
} alia_literal_palette;

// flat table size for `alia_palette` - The named layout occupies a prefix of
// this. The remaining slots are unused by the palette generator.
#define ALIA_PALETTE_SLOT_COUNT 256

// `alia_palette` is the theme color table: a named layout overlaid on a flat
// sRGB array for indexed access from styles.
typedef union alia_palette
{
    struct
    {
        alia_foundation foundation;

        alia_swatch focus;
        alia_swatch selection;

        alia_swatch primary;
        alia_swatch secondary;
        alia_swatch success;
        alia_swatch warning;
        alia_swatch danger;
        alia_swatch info;

        alia_literal_palette colors;
    };
    alia_srgb8 flat[ALIA_PALETTE_SLOT_COUNT];
} alia_palette;

#define ALIA_PALETTE_SLOT_SIZE (sizeof(alia_srgb8))

// slots per foundation ramp (must match layout)
#define ALIA_PALETTE_FOUNDATION_RAMP_STRIDE                                   \
    ((size_t) (sizeof(alia_foundation_ramp) / ALIA_PALETTE_SLOT_SIZE))
// slots per swatch (must match layout)
#define ALIA_PALETTE_SWATCH_STRIDE                                            \
    ((size_t) (sizeof(alia_swatch) / ALIA_PALETTE_SLOT_SIZE))

// flat index of the first foundation ramp slot
#define ALIA_PALETTE_INDEX_FOUNDATION_BASE                                    \
    ((size_t) (offsetof(alia_palette, foundation.background)                  \
               / ALIA_PALETTE_SLOT_SIZE))
// flat index of the first semantic swatch slot
#define ALIA_PALETTE_INDEX_SWATCH_BASE                                        \
    ((size_t) (offsetof(alia_palette, focus) / ALIA_PALETTE_SLOT_SIZE))
// flat index of the first literal swatch slot
#define ALIA_PALETTE_INDEX_LITERAL_BASE                                       \
    ((size_t) (offsetof(alia_palette, colors.red) / ALIA_PALETTE_SLOT_SIZE))

// level within a foundation ramp (weaker_4 .. base .. stronger_4)
enum alia_palette_ramp_level
{
    ALIA_PALETTE_RAMP_LEVEL_WEAKER_4 = 0,
    ALIA_PALETTE_RAMP_LEVEL_WEAKER_3,
    ALIA_PALETTE_RAMP_LEVEL_WEAKER_2,
    ALIA_PALETTE_RAMP_LEVEL_WEAKER_1,
    ALIA_PALETTE_RAMP_LEVEL_BASE,
    ALIA_PALETTE_RAMP_LEVEL_STRONGER_1,
    ALIA_PALETTE_RAMP_LEVEL_STRONGER_2,
    ALIA_PALETTE_RAMP_LEVEL_STRONGER_3,
    ALIA_PALETTE_RAMP_LEVEL_STRONGER_4,
};

// which foundation ramp (background, structural, or text)
enum alia_palette_foundation_ramp
{
    ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND = 0,
    ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
    ALIA_PALETTE_FOUNDATION_RAMP_TEXT,
};

// which part of a swatch (solid, outline, text, etc.)
enum alia_palette_swatch_part
{
    ALIA_PALETTE_SWATCH_PART_SOLID = 0,
    ALIA_PALETTE_SWATCH_PART_ON_SOLID,
    ALIA_PALETTE_SWATCH_PART_SUBTLE,
    ALIA_PALETTE_SWATCH_PART_ON_SUBTLE,
    ALIA_PALETTE_SWATCH_PART_OUTLINE,
    ALIA_PALETTE_SWATCH_PART_TEXT,
};

// semantic swatch role (focus, primary, danger, etc.)
enum alia_palette_swatch
{
    ALIA_PALETTE_SWATCH_FOCUS = 0,
    ALIA_PALETTE_SWATCH_SELECTION,
    ALIA_PALETTE_SWATCH_PRIMARY,
    ALIA_PALETTE_SWATCH_SECONDARY,
    ALIA_PALETTE_SWATCH_SUCCESS,
    ALIA_PALETTE_SWATCH_WARNING,
    ALIA_PALETTE_SWATCH_DANGER,
    ALIA_PALETTE_SWATCH_INFO,
};

// literal hue name (red, orange, blue, etc.)
enum alia_palette_literal
{
    ALIA_PALETTE_LITERAL_RED = 0,
    ALIA_PALETTE_LITERAL_ORANGE,
    ALIA_PALETTE_LITERAL_AMBER,
    ALIA_PALETTE_LITERAL_YELLOW,
    ALIA_PALETTE_LITERAL_LIME,
    ALIA_PALETTE_LITERAL_GREEN,
    ALIA_PALETTE_LITERAL_TEAL,
    ALIA_PALETTE_LITERAL_CYAN,
    ALIA_PALETTE_LITERAL_BLUE,
    ALIA_PALETTE_LITERAL_INDIGO,
    ALIA_PALETTE_LITERAL_PURPLE,
    ALIA_PALETTE_LITERAL_PINK,
};

// Return the flat index of a foundation ramp level.
static inline uint8_t
alia_palette_index_foundation_ramp(
    enum alia_palette_foundation_ramp ramp, enum alia_palette_ramp_level level)
{
    return (uint8_t) (ALIA_PALETTE_INDEX_FOUNDATION_BASE
                      + (size_t) ramp * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
                      + (size_t) level);
}

// Return the flat index of a semantic swatch part.
static inline uint8_t
alia_palette_index_swatch(
    enum alia_palette_swatch swatch, enum alia_palette_swatch_part part)
{
    return (uint8_t) (ALIA_PALETTE_INDEX_SWATCH_BASE
                      + (size_t) swatch * ALIA_PALETTE_SWATCH_STRIDE
                      + (size_t) part);
}

// Return the flat index of a literal swatch part.
static inline uint8_t
alia_palette_index_literal(
    enum alia_palette_literal literal, enum alia_palette_swatch_part part)
{
    return (uint8_t) (ALIA_PALETTE_INDEX_LITERAL_BASE
                      + (size_t) literal * ALIA_PALETTE_SWATCH_STRIDE
                      + (size_t) part);
}

// palette-backed color reference (flat index + alpha)
typedef struct alia_palette_color
{
    uint8_t index;
    uint8_t alpha;
} alia_palette_color;

// Construct a palette color from `index` and `alpha`.
static inline alia_palette_color
alia_palette_color_make(uint8_t index, uint8_t alpha)
{
    alia_palette_color c;
    c.index = index;
    c.alpha = alpha;
    return c;
}

// flat index of a named palette slot
#define ALIA_PALETTE_INDEX(path)                                              \
    ((uint8_t) (offsetof(alia_palette, path) / ALIA_PALETTE_SLOT_SIZE))

// palette color from a named slot and alpha
#define ALIA_PALETTE_COLOR(path, alpha)                                       \
    alia_palette_color_make(ALIA_PALETTE_INDEX(path), (uint8_t) (alpha))

// Construct a palette color from a foundation ramp level and `alpha`.
static inline alia_palette_color
alia_palette_color_foundation(
    enum alia_palette_foundation_ramp ramp,
    enum alia_palette_ramp_level level,
    uint8_t alpha)
{
    return alia_palette_color_make(
        alia_palette_index_foundation_ramp(ramp, level), alpha);
}

// Construct a palette color from a semantic swatch part and `alpha`.
static inline alia_palette_color
alia_palette_color_swatch(
    enum alia_palette_swatch swatch,
    enum alia_palette_swatch_part part,
    uint8_t alpha)
{
    return alia_palette_color_make(
        alia_palette_index_swatch(swatch, part), alpha);
}

// Construct a palette color from a literal swatch part and `alpha`.
static inline alia_palette_color
alia_palette_color_literal(
    enum alia_palette_literal literal,
    enum alia_palette_swatch_part part,
    uint8_t alpha)
{
    return alia_palette_color_make(
        alia_palette_index_literal(literal, part), alpha);
}

// Return the sRGB color at `index` in `p`.
static inline alia_srgb8
alia_palette_srgb_at(alia_palette const* p, uint8_t index)
{
    return p->flat[index];
}

// Resolve `index` and `alpha` against `p` to sRGBA.
static inline alia_srgba8
alia_palette_color_at(alia_palette const* p, uint8_t index, uint8_t alpha)
{
    return alia_srgba8_from_srgb8_alpha(p->flat[index], alpha);
}

// Resolve `c` against `p` to sRGBA.
static inline alia_srgba8
alia_palette_color_resolve(alia_palette const* p, alia_palette_color c)
{
    return alia_palette_color_at(p, c.index, c.alpha);
}

// operation applied to a base palette color for one interaction state
enum alia_color_op_kind
{
    // base color unchanged - This is here primarily to serve as a zero-value
    // default.
    ALIA_COLOR_OP_NONE = 0,
    // shift OKLab lightness and chroma of the base
    ALIA_COLOR_OP_ADJUST,
    // paint `color` instead of the base
    ALIA_COLOR_OP_REPLACE,
    // composite `color` over the base using a linear blend
    ALIA_COLOR_OP_BLEND,
    // shift lightness relative to the UI light/dark mode and scale chroma -
    // Positive `l` moves the color towards the foreground color.
    ALIA_COLOR_OP_STRENGTHEN,
    // move lightness and chroma partway toward the current background color
    // and take the background color's hue - `l` and `c` are fractions
    // specifying how far to move towards the background L and C values
    // (0 - not at all, 255 - all the way).
    ALIA_COLOR_OP_FADE,
};

// one interaction response applied to a base palette color
typedef struct alia_color_op
{
    // which operation
    uint8_t kind;
    union
    {
        struct
        {
            // lightness delta, in units of 1/255 - `ALIA_COLOR_OP_ADJUST` adds
            // this to OKLab lightness. `ALIA_COLOR_OP_STRENGTHEN` treats
            // positive as stronger (away from the page).
            int8_t l;
            // OKLab chroma scale - 255 leaves chroma unchanged. 0 removes it.
            uint8_t c_scale;
        } adjust;
        struct
        {
            // fraction of the lightness movement toward the background
            uint8_t l;
            // fraction of the chroma movement toward the background
            uint8_t c;
        } fade;
        // replacement or blend source
        alia_palette_color color;
    };
} alia_color_op;

// Return an op that leaves the base unchanged.
static inline alia_color_op
alia_color_op_none(void)
{
    alia_color_op op;
    op.kind = ALIA_COLOR_OP_NONE;
    op.color = alia_palette_color_make(0, 0);
    return op;
}

// Return an op that adds `l` to OKLab lightness and scales chroma by
// `c_scale`. `c_scale` is in units of 1/255, so 255 leaves chroma unchanged.
static inline alia_color_op
alia_color_op_adjust(int8_t l, uint8_t c_scale)
{
    alia_color_op op;
    op.kind = ALIA_COLOR_OP_ADJUST;
    op.adjust.l = l;
    op.adjust.c_scale = c_scale;
    return op;
}

static inline int8_t
alia_color_op_magnitude(uint8_t amount)
{
    return amount > 127 ? (int8_t) 127 : (int8_t) amount;
}

// Return an op that lightens the base by `amount` and scales chroma by
// `c_scale`. `amount` is in units of 1/255. `c_scale` matches
// `alia_color_op_adjust`.
static inline alia_color_op
alia_color_op_lighten(uint8_t amount, uint8_t c_scale)
{
    return alia_color_op_adjust(alia_color_op_magnitude(amount), c_scale);
}

// Return an op that darkens the base by `amount` and scales chroma by
// `c_scale`. Same units as `alia_color_op_lighten`.
static inline alia_color_op
alia_color_op_darken(uint8_t amount, uint8_t c_scale)
{
    return alia_color_op_adjust(
        (int8_t) -alia_color_op_magnitude(amount), c_scale);
}

// Return an op that moves the base towards the foreground color by `amount`
// and scales chroma by `c_scale`. Same units as `alia_color_op_lighten`.
static inline alia_color_op
alia_color_op_strengthen(uint8_t amount, uint8_t c_scale)
{
    alia_color_op op;
    op.kind = ALIA_COLOR_OP_STRENGTHEN;
    op.adjust.l = alia_color_op_magnitude(amount);
    op.adjust.c_scale = c_scale;
    return op;
}

// Return an op that moves the base toward the background color by `amount` and
// scales chroma by `c_scale`. Same units as `alia_color_op_lighten`.
static inline alia_color_op
alia_color_op_weaken(uint8_t amount, uint8_t c_scale)
{
    alia_color_op op;
    op.kind = ALIA_COLOR_OP_STRENGTHEN;
    op.adjust.l = (int8_t) -alia_color_op_magnitude(amount);
    op.adjust.c_scale = c_scale;
    return op;
}

// Return an op that fades lightness and chroma toward the page and takes the
// page's hue. `l` and `c` are fractions of the remaining gap. 0 stays and
// 255 arrives.
static inline alia_color_op
alia_color_op_fade(uint8_t l, uint8_t c)
{
    alia_color_op op;
    op.kind = ALIA_COLOR_OP_FADE;
    op.fade.l = l;
    op.fade.c = c;
    return op;
}

// Return an op that paints `color` instead of the base.
static inline alia_color_op
alia_color_op_replace(alia_palette_color color)
{
    alia_color_op op;
    op.kind = ALIA_COLOR_OP_REPLACE;
    op.color = color;
    return op;
}

// Return an op that composites `color` over the base.
static inline alia_color_op
alia_color_op_blend(alia_palette_color color)
{
    alia_color_op op;
    op.kind = ALIA_COLOR_OP_BLEND;
    op.color = color;
    return op;
}

// palette color plus its hover, active, and disabled responses
typedef struct alia_responsive_color
{
    // color at rest
    alia_palette_color base;
    // response while hovered
    alia_color_op hovered;
    // response while active (pressed)
    alia_color_op active;
    // response while disabled
    alia_color_op disabled;
} alia_responsive_color;

// Return a responsive color that fades `base` toward the page when disabled.
// Hover and active leave `base` unchanged.
static inline alia_responsive_color
alia_responsive_color_faded(alia_palette_color base)
{
    // fraction of the gap closed - Lightness partway, chroma all the way.
    uint8_t const l = 0xa0;
    uint8_t const c = 0xff;
    alia_responsive_color color;
    color.base = base;
    color.hovered = alia_color_op_none();
    color.active = alia_color_op_none();
    color.disabled = alia_color_op_fade(l, c);
    return color;
}

// Resolve `op` applied to `base` against `p`.
alia_srgba8
alia_color_op_resolve(
    alia_palette const* p, alia_palette_color base, alia_color_op op);

// Resolve `color` for an interaction `status`.
// `status` uses the `ALIA_INTERACTION_STATUS_*` bits. Disabled wins, then
// active, then hovered. Focus is ignored.
alia_srgba8
alia_responsive_color_resolve(
    alia_palette const* p, alia_responsive_color color, uint8_t status);

// Palette generation is a two-phase process that turns an accent color into
// theme colors and then expands them into a full theme palette. Each phase
// is parameterized, and tweaking of both intermediate and final outputs is
// supported.

// primary accent for theme color generation
typedef struct alia_theme_accent
{
    alia_oklch primary;
} alia_theme_accent;

// conditions for evaluating a palette - These are external constraints and
// contextual factors that affect palette generation. (i.e., These are the
// non-stylistic inputs.)
typedef struct alia_palette_conditions
{
    bool is_dark_mode;
    // elevation step affecting background and text lightness
    int elevation;
    // minimum contrast ratio for on-solid pairs - This can be 0 to use the
    // built-in default.
    float contrast_target;
} alia_palette_conditions;

// one value for light mode and one for dark mode
typedef struct alia_light_dark_pair
{
    float light;
    float dark;
} alia_light_dark_pair;

// recipe for generating theme colors from an accent color
typedef struct alia_theme_colors_recipe
{
    // base background lightness
    alia_light_dark_pair bg_l;
    // background lightness change per elevation step
    float bg_elevation_step;
    // background chroma
    alia_light_dark_pair bg_c;

    // base text lightness
    alia_light_dark_pair text_l;
    // text chroma
    float text_c;

    // accent fill lightness - Hue and chroma come from the accent.
    alia_light_dark_pair accent_l;
    // chroma used when an accent is built from a hue alone
    float accent_c_default;

    // shared lightness for danger, warning, success, and info
    alia_light_dark_pair alert_l;
    // shared chroma for the alert hues
    float alert_c;
} alia_theme_colors_recipe;

// OKLCH colors chosen for the theme
typedef struct alia_theme_colors
{
    alia_oklch bg;
    alia_oklch structural;
    alia_oklch text;
    alia_oklch primary;
    alia_oklch secondary;
    alia_oklch success;
    alia_oklch warning;
    alia_oklch danger;
    alia_oklch info;
} alia_theme_colors;

// how literal hues relate to the primary hue
enum alia_palette_literal_policy
{
    // fixed hue table
    ALIA_PALETTE_LITERAL_FIXED_SPECTRUM = 0,
    // harmonized relative to the primary palette color - Each literal hue is
    // shifted so that it is a multiple of 15 degrees away from the primary
    // hue.
    ALIA_PALETTE_LITERAL_HARMONIZE_TO_PRIMARY,
};

// recipe for expanding one OKLCH color into a ramp or a swatch
typedef struct alia_palette_expansion
{
    float foundation_step_l;

    // soft-fill lightness
    alia_light_dark_pair swatch_subtle_l;
    // emphasized-text lightness
    alia_light_dark_pair swatch_text_l;
    // dark ink used when the solid is light
    float on_solid_l_for_light_solid;
    // light ink used when the solid is dark
    float on_solid_l_for_dark_solid;
    float swatch_outline_c_scale;
    float swatch_text_c_scale;
    float swatch_subtle_c;

    // literal hue lightness
    alia_light_dark_pair literal_l;
    float literal_c;
    // literal hue placement relative to the primary
    enum alia_palette_literal_policy literal_policy;
} alia_palette_expansion;

// Return default palette conditions for a given UI lightness mode.
alia_palette_conditions
alia_palette_conditions_default(bool is_dark_mode);

// Return the default recipe for generating theme colors.
alia_theme_colors_recipe
alia_theme_colors_recipe_default(void);

// Return the default palette expansion parameters.
alia_palette_expansion
alia_palette_expansion_default(void);

// Fill `out` with an accent derived from `hue`.
void
alia_theme_accent_from_hue(
    alia_theme_accent* out, float hue, alia_theme_colors_recipe const* recipe);

// Fill `out` with an accent derived from `color`.
void
alia_theme_accent_from_color(alia_theme_accent* out, alia_srgb8 color);

// Fill `out` with theme colors derived from `accent`.
// `recipe` can be null to use the default.
void
alia_theme_colors_generate(
    alia_theme_colors* out,
    alia_theme_accent const* accent,
    alia_palette_conditions const* conditions,
    alia_theme_colors_recipe const* recipe);

// Expand `colors` into an sRGB palette in `out`.
// `expansion` can be null to use the default.
void
alia_palette_generate(
    alia_palette* out,
    alia_theme_colors const* colors,
    alia_palette_expansion const* expansion,
    alia_palette_conditions const* conditions);

// Generate theme colors from `accent` and expand them into an sRGB palette in
// `out`. `recipe` and `expansion` can be null to use the defaults.
void
alia_palette_generate_from_accent(
    alia_palette* out,
    alia_theme_accent const* accent,
    alia_palette_conditions const* conditions,
    alia_theme_colors_recipe const* recipe,
    alia_palette_expansion const* expansion);

ALIA_EXTERN_C_END

#endif // ALIA_UI_PALETTE_H
