#ifndef ALIA_ABI_THEME_H
#define ALIA_ABI_THEME_H

#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>
#include <stdbool.h>
#include <stdint.h>

ALIA_EXTERN_C_BEGIN

// -----------------------------------------------------------------------------
// CAPACITY MACROS
// -----------------------------------------------------------------------------

// Structural elements get longer ramps for fine-tuning layout depth
#define ALIA_THEME_STRUCTURAL_TONE_COUNT 6

// The maximum nested depth the UI will natively pre-compute
#define ALIA_THEME_MAX_ELEVATIONS 4

// The open-ended capacity for custom application intents
#ifndef ALIA_THEME_MAX_INTENTS
#define ALIA_THEME_MAX_INTENTS 8
#endif

// Framework-guaranteed intent indices
#define ALIA_THEME_INTENT_PRIMARY 0
#define ALIA_THEME_INTENT_NEUTRAL 1
#define ALIA_THEME_INTENT_WARNING 2
#define ALIA_THEME_INTENT_ERROR 3

// -----------------------------------------------------------------------------
// AXIS 1 & 2: CONTRAST AND INTERACTION
// -----------------------------------------------------------------------------

// Used for backgrounds/fills that require mathematically guaranteed readable
// text
typedef struct
{
    alia_srgb8 color;
    alia_srgb8 on_color;
} alia_paired_color;

// The 4-state machine for elements requiring paired contrast
typedef union
{
    struct
    {
        alia_paired_color idle;
        alia_paired_color hover;
        alia_paired_color active;
        alia_paired_color disabled;
    };
    alia_paired_color states[4];
} alia_paired_interaction;

// The 4-state machine for pure foregrounds (text/borders)
typedef union
{
    struct
    {
        alia_srgb8 idle;
        alia_srgb8 hover;
        alia_srgb8 active;
        alia_srgb8 disabled;
    };
    alia_srgb8 states[4];
} alia_color_interaction;

// -----------------------------------------------------------------------------
// AXIS 3: TONAL RAMPS (Structural vs Intent)
// -----------------------------------------------------------------------------

// The elongated structural ramps for fine-tuning layout depth
typedef struct
{
    alia_paired_interaction base;
    alia_paired_interaction stronger[ALIA_THEME_STRUCTURAL_TONE_COUNT];
    alia_paired_interaction weaker[ALIA_THEME_STRUCTURAL_TONE_COUNT];
} alia_structural_paired_ramp;

typedef struct
{
    alia_color_interaction base;
    alia_color_interaction stronger[ALIA_THEME_STRUCTURAL_TONE_COUNT];
    alia_color_interaction weaker[ALIA_THEME_STRUCTURAL_TONE_COUNT];
} alia_structural_color_ramp;

// The short, punchy interaction intents (Danger, Primary, etc.)
typedef struct
{
    alia_paired_interaction solid; // High-contrast filled action
    alia_paired_interaction subtle; // Low-contrast tinted action
    alia_color_interaction text; // Pure foreground intent (links, icons)
} alia_intent_ramp;

// -----------------------------------------------------------------------------
// THE AMBIENT ENVIRONMENT (A completely self-contained palette)
// -----------------------------------------------------------------------------

typedef struct
{
    // 1. Structural Elements (Long ramps, separated semantics)
    alia_structural_paired_ramp background;
    alia_structural_color_ramp border;
    alia_structural_color_ramp text;

    // 2. Cross-cutting Geometric Overrides
    alia_srgba8 focus_ring;
    alia_srgb8 selection_bg;
    alia_srgb8 selection_text;
    alia_srgba8 ambient_shadow; // Tight, dark drop-shadow
    alia_srgba8 directional_shadow; // Softer, offset drop-shadow

    // 3. Open-Ended Intents (Inline union for 0-allocation flexibility)
    union
    {
        struct
        {
            alia_intent_ramp primary;
            alia_intent_ramp neutral;
            alia_intent_ramp warning;
            alia_intent_ramp error;
        };
        alia_intent_ramp intents[ALIA_THEME_MAX_INTENTS];
    };

} alia_surface_theme;

// -----------------------------------------------------------------------------
// THE MASTER THEME COLLECTION
// -----------------------------------------------------------------------------

typedef struct
{
    // Pre-computed palettes from deepest background [0] to highest modal [3]
    alia_surface_theme surfaces[ALIA_THEME_MAX_ELEVATIONS];

    // The escape-hatch palette for inverted components (e.g., dark tooltips)
    alia_surface_theme inverted;
} alia_theme_collection;

// -----------------------------------------------------------------------------
// DOWNSTREAM WIDGET MAPPING (Example)
// -----------------------------------------------------------------------------
// Framework-provided functions to safely map the massive LUT above into
// bite-sized, boilerplate-free structs for specific UI components.

typedef struct
{
    alia_color_interaction background;
    alia_color_interaction border;
    alia_color_interaction text;
    alia_srgba8 focus_ring;
} alia_text_input_style;

// alia_text_input_style
// alia_map_text_input(alia_surface_theme const* env);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_THEME_H */