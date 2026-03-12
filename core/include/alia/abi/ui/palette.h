#ifndef ALIA_UI_PALETTE_H
#define ALIA_UI_PALETTE_H

#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct
{
    alia_srgb8 idle;
    alia_srgb8 hover;
    alia_srgb8 active;
    alia_srgb8 disabled;
} alia_color_states;

typedef struct
{
    alia_color_states solid;
    alia_color_states subtle;
    alia_color_states outline;
    alia_color_states text;
} alia_swatch;

typedef struct
{
    alia_color_states weaker_4;
    alia_color_states weaker_3;
    alia_color_states weaker_2;
    alia_color_states weaker_1;
    alia_color_states base;
    alia_color_states stronger_1;
    alia_color_states stronger_2;
    alia_color_states stronger_3;
    alia_color_states stronger_4;
} alia_foundation_ramp;

// PALETTES

typedef struct
{
    alia_foundation_ramp background;
    alia_foundation_ramp structural;
    alia_foundation_ramp text;
} alia_foundation;

typedef struct
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

typedef union
{
    struct
    {
        alia_foundation foundation; // 27 slots

        alia_swatch primary; // 4 slots each...
        alia_swatch secondary;
        alia_swatch success;
        alia_swatch warning;
        alia_swatch danger;
        alia_swatch info; // ...24 slots total

        alia_literal_palette colors; // 32 slots

        alia_color_states focus_ring; // 3 slots
        alia_color_states selection_bg;
        alia_color_states selection_text;
    };

    // The flat memory map for O(1) rendering lookups (86 used, 170 free)
    alia_color_states roles[256];
} alia_palette;

// GENERATION

typedef struct
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

typedef struct
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
