#ifndef ALIA_ABI_UI_STYLING_PALETTE_GENERATION_H
#define ALIA_ABI_UI_STYLING_PALETTE_GENERATION_H

#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/styling/environment.h>

ALIA_EXTERN_C_BEGIN

// UI lightness mode (light or dark surface)
enum alia_ui_lightness_mode
{
    // dark text on light background
    ALIA_UI_LIGHT_MODE = 0,
    // light text on dark background
    ALIA_UI_DARK_MODE = 1
};

// recipe fields - Mode-symmetric generative knobs. Light/dark is chosen at
// `alia_palette_generate` time. The same recipe flips to an analogous palette.
//
// `X(field, "description", GROUP, min, max, step, default, default)`
// Both default columns match (descriptors share the light/dark slots).

// clang-format off
#define ALIA_PALETTE_RECIPE_FIELDS(X)                                         \
    X(hue, "primary accent hue (degrees)", RECIPE,                            \
      0.f, 360.f, 1.f, 210.f, 210.f)                                          \
    X(chroma, "accent chroma", RECIPE,                                        \
      0.f, 0.35f, 0.005f, 0.20f, 0.20f)                                       \
    X(lightness, "page lightness toward the mode pole", RECIPE,               \
      0.f, 1.f, 0.01f, 0.92f, 0.92f)                                          \
    X(contrast, "ink separation from the page", RECIPE,                       \
      0.f, 1.f, 0.01f, 0.85f, 0.85f)                                          \
    X(hue_spacing, "secondary/tertiary hue offset (degrees)", RECIPE,         \
      0.f, 180.f, 1.f, 35.f, 35.f)
// clang-format on

typedef struct alia_palette_recipe
{
#define ALIA_PALETTE_RECIPE_FIELD_X(                                          \
    NAME, desc, group, min, max, step, dl, dd)                                \
    float NAME;
    ALIA_PALETTE_RECIPE_FIELDS(ALIA_PALETTE_RECIPE_FIELD_X)
#undef ALIA_PALETTE_RECIPE_FIELD_X
} alia_palette_recipe;

// how `alia_palette_from_swatches` assigns chromatic seeds
typedef enum alia_swatch_assign
{
    // colors[] order: accents first, then optional neutrals
    ALIA_SWATCH_ASSIGN_ORDER = 0,
    // rank by chroma into primary / secondary / tertiary (neutrals by L)
    ALIA_SWATCH_ASSIGN_AUTO = 1
} alia_swatch_assign;

// small recipe for swatch import
typedef struct alia_swatch_recipe
{
    alia_swatch_assign assign;
    // synthesize danger near red when no swatch maps to it
    bool synthesize_danger;
} alia_swatch_recipe;

// Return the default mode-symmetric recipe.
alia_palette_recipe
alia_palette_recipe_default(void);

// Set `recipe->hue` from `hue_degrees` (wrapped into [0, 360)).
void
alia_palette_recipe_seed_hue(alia_palette_recipe* recipe, float hue_degrees);

// Set `recipe->hue` and `recipe->chroma` from `color`. Chroma is capped; near-
// gray seeds leave chroma unchanged. Other knobs are untouched.
void
alia_palette_recipe_seed_color(alia_palette_recipe* recipe, alia_srgb8 color);

// Fill `out` colors from `recipe` and `dark`. `recipe` may be null to use
// `alia_palette_recipe_default()`. Does not touch vars or seeds.
void
alia_palette_generate(
    alia_palette* out, alia_palette_recipe const* recipe, bool dark);

// Return the default swatch recipe.
alia_swatch_recipe
alia_swatch_recipe_default(void);

// Generate a palette using an array of seed colors and a basic recipe.
void
alia_palette_from_swatches(
    alia_palette* out,
    alia_srgb8 const* colors,
    int count,
    bool dark,
    alia_swatch_recipe const* recipe);

// Shift SURFACE toward TEXT by `delta_l` (positive raises the surface). WELL
// and NEUTRAL keep their lightness offset from SURFACE. Accents and text
// stay absolute. Pass a negative `delta_l` to sink.
void
alia_palette_elevate(alia_palette* out, alia_palette const* in, float delta_l);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_STYLING_PALETTE_GENERATION_H
