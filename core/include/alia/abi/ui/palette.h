#ifndef ALIA_UI_PALETTE_H
#define ALIA_UI_PALETTE_H

#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>
#include <stddef.h> /* offsetof for index builders */

ALIA_EXTERN_C_BEGIN

enum alia_ui_lightness_mode
{
    // dark text on light background
    ALIA_UI_LIGHT_MODE = 0,
    // light text on dark background
    ALIA_UI_DARK_MODE = 1
};

typedef struct alia_swatch
{
    alia_srgb8 solid;
    alia_srgb8 on_solid;

    alia_srgb8 subtle;
    alia_srgb8 on_subtle;

    alia_srgb8 outline;
    alia_srgb8 text;
} alia_swatch;

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

// Flat index count for the palette union (padding in `flat` aliases the
// structured region only for indices that exist in the named layout).
#define ALIA_PALETTE_SLOT_COUNT 256

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

// --- Flat index API: constants + direct math (no switches). Packing is validated
//     at compile time in palette generation code via alia_palette_flat_index_ok(). ---

#define ALIA_PALETTE_SLOT_SIZE (sizeof(alia_srgb8))

// Strides (slots per sub-struct). Used in index math; must match actual layout.
#define ALIA_PALETTE_FOUNDATION_RAMP_STRIDE \
    ((size_t)(sizeof(alia_foundation_ramp) / ALIA_PALETTE_SLOT_SIZE))
#define ALIA_PALETTE_SWATCH_STRIDE \
    ((size_t)(sizeof(alia_swatch) / ALIA_PALETTE_SLOT_SIZE))

// Base indices (first slot of each region). Index = base + stride * N + part.
#define ALIA_PALETTE_INDEX_FOUNDATION_BASE \
    ((size_t)(offsetof(alia_palette, foundation.background) / ALIA_PALETTE_SLOT_SIZE))
#define ALIA_PALETTE_INDEX_SWATCH_BASE \
    ((size_t)(offsetof(alia_palette, focus) / ALIA_PALETTE_SLOT_SIZE))
#define ALIA_PALETTE_INDEX_LITERAL_BASE \
    ((size_t)(offsetof(alia_palette, colors.red) / ALIA_PALETTE_SLOT_SIZE))

// Ramp level within a foundation ramp (weaker_4 .. base .. stronger_4).
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

// Which foundation ramp (background, structural, text).
enum alia_palette_foundation_ramp
{
    ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND = 0,
    ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
    ALIA_PALETTE_FOUNDATION_RAMP_TEXT,
};

// Part of a swatch (solid, outline, text, etc.).
enum alia_palette_swatch_part
{
    ALIA_PALETTE_SWATCH_PART_SOLID = 0,
    ALIA_PALETTE_SWATCH_PART_ON_SOLID,
    ALIA_PALETTE_SWATCH_PART_SUBTLE,
    ALIA_PALETTE_SWATCH_PART_ON_SUBTLE,
    ALIA_PALETTE_SWATCH_PART_OUTLINE,
    ALIA_PALETTE_SWATCH_PART_TEXT,
};

// Semantic swatch (focus, primary, danger, etc.).
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

// Literal color name (red, orange, blue, etc.).
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

static inline uint8_t
alia_palette_index_foundation_ramp(
    enum alia_palette_foundation_ramp ramp, enum alia_palette_ramp_level level)
{
    return (uint8_t)(ALIA_PALETTE_INDEX_FOUNDATION_BASE
                     + (size_t)ramp * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
                     + (size_t)level);
}

static inline uint8_t
alia_palette_index_swatch(
    enum alia_palette_swatch swatch, enum alia_palette_swatch_part part)
{
    return (uint8_t)(ALIA_PALETTE_INDEX_SWATCH_BASE
                     + (size_t)swatch * ALIA_PALETTE_SWATCH_STRIDE
                     + (size_t)part);
}

static inline uint8_t
alia_palette_index_literal(
    enum alia_palette_literal literal, enum alia_palette_swatch_part part)
{
    return (uint8_t)(ALIA_PALETTE_INDEX_LITERAL_BASE
                     + (size_t)literal * ALIA_PALETTE_SWATCH_STRIDE
                     + (size_t)part);
}

// Index + alpha for a palette-backed color (widget styles, etc.).
typedef struct alia_palette_color
{
    uint8_t index;
    uint8_t alpha;
} alia_palette_color;

static inline alia_palette_color
alia_palette_color_make(uint8_t index, uint8_t alpha)
{
    alia_palette_color c;
    c.index = index;
    c.alpha = alpha;
    return c;
}

static inline alia_srgb8
alia_palette_srgb_at(alia_palette const* p, uint8_t index)
{
    return p->flat[index];
}

// Resolve flat index + alpha to premultiplied-friendly sRGBA.
static inline alia_srgba8
alia_palette_color_at(
    alia_palette const* p, uint8_t index, uint8_t alpha)
{
    return alia_srgba8_from_srgb8_alpha(p->flat[index], alpha);
}

static inline alia_srgba8
alia_palette_color_resolve(
    alia_palette const* p, alia_palette_color c)
{
    return alia_palette_color_at(p, c.index, c.alpha);
}

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
