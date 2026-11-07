#ifndef ALIA_ABI_UI_STYLING_ENVIRONMENT_H
#define ALIA_ABI_UI_STYLING_ENVIRONMENT_H

#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_style_context alia_style_context;

// STYLE COLOR SLOTS (palette)
//
// Dense indices into `alia_palette.colors`. Core slots are structural + accent
// seeds + danger. App-defined colors start at `ALIA_STYLE_COLOR_APP_BEGIN`.
//
// `X(NAME, "description", GROUP)` - GROUP is a suffix of ALIA_STYLE_GROUP_*.

// clang-format off
#define ALIA_STYLE_COLORS(X)                                                  \
    X(SURFACE, "page / panel background", STRUCTURAL)                         \
    X(NEUTRAL, "quiet chrome / secondary ink", STRUCTURAL)                    \
    X(TEXT, "body ink", STRUCTURAL)                                           \
    X(WELL, "shared control-surface token", STRUCTURAL)                       \
    X(FOCUS, "keyboard focus ring", ACCENT)                                   \
    X(PRIMARY, "brand accent", ACCENT)                                        \
    X(SECONDARY, "secondary accent", ACCENT)                                  \
    X(TERTIARY, "tertiary accent", ACCENT)                                    \
    X(DANGER, "destructive / error accent", ACCENT)
// clang-format on

typedef uint8_t alia_style_color_slot;

enum
{
#define ALIA_STYLE_COLOR_ENUM_X(NAME, desc, group) ALIA_STYLE_COLOR_##NAME,
    ALIA_STYLE_COLORS(ALIA_STYLE_COLOR_ENUM_X)
#undef ALIA_STYLE_COLOR_ENUM_X
        ALIA_STYLE_COLOR_CORE_COUNT,
    ALIA_STYLE_COLOR_APP_BEGIN = 64,
};

#define ALIA_STYLE_COLOR_COUNT 128

// where a style recipe takes a single OKLCH channel from
typedef uint8_t alia_style_channel_source_t;

enum
{
    // take the channel from a palette slot
    ALIA_STYLE_CHANNEL_SLOT = 0,
    // take the channel from an immediate (`value`)
    ALIA_STYLE_CHANNEL_VALUE = 1,
};

// reference for one OKLCH channel (H or C) in a style recipe
typedef struct alia_style_channel_ref
{
    alia_style_channel_source_t source;
    // palette slot when `source` is SLOT
    alia_style_color_slot slot;
    // immediate when `source` is VALUE - Hue is degrees; chroma is OKLCH C.
    float value;
} alia_style_channel_ref;

static inline alia_style_channel_ref
alia_style_channel_from_slot(alia_style_color_slot slot)
{
    alia_style_channel_ref ref;
    ref.source = ALIA_STYLE_CHANNEL_SLOT;
    ref.slot = slot;
    ref.value = 0.f;
    return ref;
}

static inline alia_style_channel_ref
alia_style_channel_from_value(float value)
{
    alia_style_channel_ref ref;
    ref.source = ALIA_STYLE_CHANNEL_VALUE;
    ref.slot = 0;
    ref.value = value;
    return ref;
}

// OKLCH color table sampled by LOAD_COLOR - sRGB is emitted at resolve time.
typedef struct alia_palette
{
    alia_oklch colors[ALIA_STYLE_COLOR_COUNT];
} alia_palette;

// STYLE VAR SLOTS
//
// Named scalars for LOAD_VAR. Apps may use indices at
// `ALIA_STYLE_VAR_APP_BEGIN` and above.
//
// `X(NAME, "description", GROUP, min, max, step, default_light, default_dark)`

// clang-format off
#define ALIA_STYLE_VARS(X)                                                    \
    X(MATCH_STEP, "SURFACE-to-TEXT L step for match tone", TONE,              \
      0.f, 1.f, 0.01f, 0.14f, 0.18f)                                          \
    X(INVERSE_STEP, "SURFACE-to-TEXT L step for inverse tone", TONE,          \
      0.f, 1.f, 0.01f, 0.68f, 0.72f)                                          \
    X(MATCH_CHROMA, "chroma scale for match tone", TONE,                      \
      0.f, 1.5f, 0.01f, 0.65f, 0.72f)                                         \
    X(INVERSE_CHROMA, "chroma scale for inverse tone", TONE,                  \
      0.f, 1.5f, 0.01f, 0.90f, 0.92f)                                         \
    X(DARK_L, "absolute L for dark tone", TONE,                               \
      0.f, 1.f, 0.01f, 0.42f, 0.42f)                                          \
    X(LIGHT_L, "absolute L for light tone", TONE,                             \
      0.f, 1.f, 0.01f, 0.88f, 0.88f)                                          \
    X(ABSOLUTE_CHROMA, "chroma scale for absolute dark/light tones", TONE,    \
      0.f, 1.5f, 0.01f, 0.85f, 0.85f)                                         \
    X(DISABLED_FADE, "lerp toward SURFACE when disabled", INTERACTION,        \
      0.f, 1.f, 0.01f, 0xa0 / 255.f, 0xa0 / 255.f)                            \
    X(HOVER_STRENGTHEN, "hover L strengthen toward TEXT", INTERACTION,        \
      0.f, 0.3f, 0.005f, 0.05f, 0.05f)                                        \
    X(ACTIVE_STRENGTHEN, "active L strengthen toward TEXT", INTERACTION,      \
      0.f, 0.3f, 0.005f, 0.1f, 0.1f)                                          \
    X(GLOW_L, "drop-shadow / glow L-step", INTERACTION,                       \
      0.f, 1.f, 0.01f, 0.30f, 0.30f)                                          \
    X(HIGHLIGHT_L, "hover highlight L-step over SURFACE", INTERACTION,        \
      0.f, 0.3f, 0.005f, 0.05f, 0.05f)                                        \
    X(CONTRAST_TARGET, "base contrast ratio for ensure_contrast", INTERACTION,\
      1.f, 12.f, 0.25f, 4.5f, 4.5f)                                           \
    X(FLARE_PEAK, "click flare peak vs highlight baseline", INTERACTION,      \
      0.f, 4.f, 0.05f, 2.25f, 2.25f)
// clang-format on

typedef uint8_t alia_style_var_slot;

#define ALIA_STYLE_VAR_COUNT 64
#define ALIA_STYLE_VAR_APP_BEGIN 32

enum
{
#define ALIA_STYLE_VAR_ENUM_X(NAME, desc, group, min, max, step, dl, dd)      \
    ALIA_STYLE_VAR_##NAME,
    ALIA_STYLE_VARS(ALIA_STYLE_VAR_ENUM_X)
#undef ALIA_STYLE_VAR_ENUM_X
        ALIA_STYLE_VAR_CORE_COUNT
};

// STYLE SEEDS (generation-time geometry inputs)
//
// These shape style generation (layout rhythm and chrome size). They are part
// of the environment but have no LOAD opcode; generators bake them as
// immediates or sizes.
//
// `X(field, "description", GROUP, min, max, step, default_light, default_dark)`

// clang-format off
#define ALIA_STYLE_SEEDS(X)                                                   \
    X(spacing, "layout rhythm (sibling gap), logical px", GEOMETRY,           \
      0.f, 24.f, 1.f, 10.f, 10.f)                                             \
    X(scale, "control chrome size multiplier", GEOMETRY,                      \
      0.25f, 3.f, 0.05f, 1.f, 1.f)                                            \
    X(roundness, "corner language from square (0) to round (1)", GEOMETRY,    \
      0.f, 1.f, 0.05f, 1.f, 1.f)                                              \
    X(outline_width, "outline chrome stroke, logical px", GEOMETRY,           \
      0.f, 8.f, 0.25f, 1.5f, 1.5f)
// clang-format on

typedef struct alia_style_seeds
{
#define ALIA_STYLE_SEED_FIELD_X(NAME, desc, group, min, max, step, dl, dd)    \
    float NAME;
    ALIA_STYLE_SEEDS(ALIA_STYLE_SEED_FIELD_X)
#undef ALIA_STYLE_SEED_FIELD_X
} alia_style_seeds;

// Return the default style seeds.
alia_style_seeds
alia_style_seeds_default(void);

// Set the ambient style fields on `style` from `seeds`.
// `seeds` may be NULL to use the default seeds.
void
alia_style_apply_seeds(
    alia_style_context* style, alia_style_seeds const* seeds);

// `alia_style_environment` is the resolve-time and generation-time styling
// environment: palette colors, vars for LOAD_VAR, and seeds for generators.
typedef struct alia_style_environment
{
    alia_palette palette;
    float vars[ALIA_STYLE_VAR_COUNT];
    alia_style_seeds seeds;
} alia_style_environment;

// Return a style var, or 0 if `index` is out of range.
static inline float
alia_style_var(alia_style_environment const* env, uint8_t index)
{
    return index < ALIA_STYLE_VAR_COUNT ? env->vars[index] : 0.f;
}

// Return the OKLCH color for `slot`.
static inline alia_oklch
alia_palette_color_oklch(alia_palette const* p, uint8_t slot)
{
    return p->colors[slot];
}

// Return `slot` clipped to sRGB.
static inline alia_srgb8
alia_palette_color_srgb(alia_palette const* p, uint8_t slot)
{
    return alia_srgb8_from_unclamped_oklch(alia_palette_color_oklch(p, slot));
}

// Return `slot` clipped to opaque sRGBA.
static inline alia_srgba8
alia_palette_color_srgba(alia_palette const* p, uint8_t slot)
{
    return alia_srgba8_from_srgb8(alia_palette_color_srgb(p, slot));
}

static inline alia_oklch
alia_style_color_oklch(alia_style_environment const* env, uint8_t slot)
{
    return alia_palette_color_oklch(&env->palette, slot);
}

static inline alia_srgb8
alia_style_color_srgb(alia_style_environment const* env, uint8_t slot)
{
    return alia_palette_color_srgb(&env->palette, slot);
}

static inline alia_srgba8
alia_style_color_srgba(alia_style_environment const* env, uint8_t slot)
{
    return alia_palette_color_srgba(&env->palette, slot);
}

// Return soft overlay coverage for `fg_slot` over SURFACE using `l_step`.
// Caps `l_step` to half of `|fg_l - bg_l|` so elevated surfaces cannot force
// opaque coverage, then delegates to `alia_oklch_overlay_alpha`.
static inline float
alia_style_overlay_alpha(
    alia_style_environment const* env, uint8_t fg_slot, float l_step)
{
    alia_oklch const bg
        = alia_style_color_oklch(env, ALIA_STYLE_COLOR_SURFACE);
    alia_oklch const fg = alia_style_color_oklch(env, fg_slot);
    float step = l_step;
    if (step < 0.f)
        step = 0.f;
    float headroom = fg.l - bg.l;
    if (headroom < 0.f)
        headroom = -headroom;
    float const max_step = 0.5f * headroom;
    if (step > max_step)
        step = max_step;
    return alia_oklch_overlay_alpha(bg.l, fg.l, step);
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_STYLING_ENVIRONMENT_H
