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

// Theme generation expands an accent through OKLCH seeds into an sRGB
// palette. Apps can hold and edit values at any stage before the next step.

// primary accent for theme generation
typedef struct alia_theme_accent
{
    alia_oklch primary;
} alia_theme_accent;

// surface environment for theme generation
typedef struct alia_surface_env
{
    bool is_dark_mode;
    // elevation step affecting background and text lightness
    int elevation;
} alia_surface_env;

// context for expanding an accent into seeds and palette
typedef struct alia_theme_context
{
    alia_surface_env surface;
    // minimum contrast ratio for on-solid pairs - A value of 0 uses generator
    // defaults.
    float contrast_target;
} alia_theme_context;

// OKLCH seeds expanded from an accent before sRGB palette generation
typedef struct alia_palette_seeds
{
    alia_oklch bg;
    alia_oklch text;
    alia_oklch primary;
    alia_oklch secondary;
    alia_oklch success;
    alia_oklch warning;
    alia_oklch danger;
    alia_oklch info;
} alia_palette_seeds;

// parameters controlling accent-to-seed expansion
typedef struct alia_seed_params
{
    float primary_l_default;
    float primary_c_default;

    float elevation_bg_l_base_dark;
    float elevation_bg_l_per_step_dark;
    float elevation_bg_l_base_light;
    float elevation_bg_l_per_step_light;
    float elevation_bg_c;
    float elevation_text_l_dark;
    float elevation_text_l_light;
    float elevation_text_c;

    float secondary_l_offset;
    float secondary_c_offset;

    float alert_l_dark;
    float alert_l_light;
    float alert_c;
    float alert_hue_danger_deg;
    float alert_hue_warning_deg;
    float alert_hue_success_deg;
    float alert_hue_info_deg;
    float alert_warning_l_offset;
} alia_seed_params;

// parameters controlling seed-to-palette expansion
typedef struct alia_palette_params
{
    float foundation_step_l;
    float structural_l_offset;
    float structural_c_offset;

    float swatch_subtle_l_dark;
    float swatch_subtle_l_light;
    float swatch_text_l_dark;
    float swatch_text_l_light;
    float swatch_on_solid_l_dark;
    float swatch_on_solid_l_light;
    float swatch_outline_c_scale;
    float swatch_text_c_scale;
    float swatch_subtle_c;

    float literal_l_dark;
    float literal_l_light;
    float literal_c;
} alia_palette_params;

// how literal hue swatches are generated
enum alia_literal_policy
{
    ALIA_LITERAL_FIXED_SPECTRUM = 0,
    ALIA_LITERAL_HARMONIZE_TO_PRIMARY,
};

// Return a default theme context for `is_dark_mode`.
alia_theme_context
alia_theme_context_default(bool is_dark_mode);

// Return the default seed expansion parameters.
alia_seed_params
alia_seed_params_default(void);

// Return the default palette expansion parameters.
alia_palette_params
alia_palette_params_default(void);

// Fill `out` with an accent derived from `hue`.
void
alia_theme_accent_from_hue(
    alia_theme_accent* out, float hue, alia_seed_params const* params);

// Fill `out` with an accent derived from `color`.
void
alia_theme_accent_from_color(alia_theme_accent* out, alia_srgb8 color);

// Expand `accent` into OKLCH seeds in `out`.
void
alia_palette_seeds_from_accent(
    alia_palette_seeds* out,
    alia_theme_accent const* accent,
    alia_theme_context const* ctx,
    alia_seed_params const* params);

// Expand `seeds` into an sRGB palette in `out`.
void
alia_palette_from_seeds(
    alia_palette* out,
    alia_palette_seeds const* seeds,
    alia_theme_context const* ctx,
    alia_palette_params const* params,
    enum alia_literal_policy literals);

// Expand `accent` into an sRGB palette in `out`.
void
alia_palette_from_accent(
    alia_palette* out,
    alia_theme_accent const* accent,
    alia_theme_context const* ctx,
    alia_seed_params const* seed_params,
    alia_palette_params const* palette_params,
    enum alia_literal_policy literals);

ALIA_EXTERN_C_END

#endif // ALIA_UI_PALETTE_H
