#ifndef ALIA_UI_PALETTE_H
#define ALIA_UI_PALETTE_H

#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

enum alia_ui_lightness_mode
{
    // dark text on light background
    ALIA_UI_LIGHT_MODE = 0,
    // light text on dark background
    ALIA_UI_DARK_MODE = 1
};

typedef struct alia_interaction_colors
{
    alia_srgb8 idle;
    alia_srgb8 hover;
    alia_srgb8 active;
    alia_srgb8 disabled;
} alia_interaction_colors;

typedef struct alia_swatch
{
    alia_interaction_colors solid;
    alia_interaction_colors on_solid;

    alia_interaction_colors subtle;
    alia_interaction_colors on_subtle;

    alia_interaction_colors outline;
    alia_interaction_colors text;
} alia_swatch;

typedef struct alia_foundation_ramp
{
    alia_interaction_colors weaker_4;
    alia_interaction_colors weaker_3;
    alia_interaction_colors weaker_2;
    alia_interaction_colors weaker_1;
    alia_interaction_colors base;
    alia_interaction_colors stronger_1;
    alia_interaction_colors stronger_2;
    alia_interaction_colors stronger_3;
    alia_interaction_colors stronger_4;
} alia_foundation_ramp;

// PALETTES

typedef struct alia_foundation
{
    alia_foundation_ramp background;
    alia_foundation_ramp structural;
    alia_foundation_ramp text;
} alia_foundation;

typedef struct alia_literal_palette
{
    alia_swatch red;
    alia_swatch orange;
    alia_swatch yellow;
    alia_swatch green;
    alia_swatch cyan;
    alia_swatch blue;
    alia_swatch purple;
    alia_swatch pink;
} alia_literal_palette;

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

    // The flat memory map for O(1) rendering lookups (86 used, 170 free)
    alia_interaction_colors roles[256];
} alia_palette;

// GENERATION

typedef struct alia_palette_seeds
{
    alia_srgb8 bg_base;
    alia_srgb8 text_base;
    alia_srgb8 primary;
    alia_srgb8 secondary;
    alia_srgb8 success;
    alia_srgb8 warning;
    alia_srgb8 danger;
    alia_srgb8 info;
} alia_palette_seeds;

typedef struct alia_theme_params
{
    float foundation_step_l;
    float hover_l_shift;
    float active_l_shift;
    float interaction_hue_shift;
    bool is_dark_mode;
} alia_theme_params;

alia_palette_seeds
alia_seeds_from_elevation(alia_srgb8 brand, int elevation, bool is_dark);

alia_palette_seeds
alia_seeds_from_core(
    alia_srgb8 brand, alia_srgb8 bg, alia_srgb8 text, bool is_dark);

void
alia_palette_expand(
    alia_palette* out_palette,
    const alia_palette_seeds* seeds,
    const alia_theme_params* params);

ALIA_EXTERN_C_END

#endif // ALIA_UI_PALETTE_H
