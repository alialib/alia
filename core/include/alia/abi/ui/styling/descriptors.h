#ifndef ALIA_ABI_UI_STYLING_DESCRIPTORS_H
#define ALIA_ABI_UI_STYLING_DESCRIPTORS_H

#include <alia/abi/prelude.h>

#include <stddef.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// section IDs for grouping descriptors in inspector UIs
enum
{
    ALIA_STYLE_GROUP_STRUCTURAL = 0,
    ALIA_STYLE_GROUP_ACCENT = 1,
    ALIA_STYLE_GROUP_TONE = 2,
    ALIA_STYLE_GROUP_INTERACTION = 3,
    ALIA_STYLE_GROUP_GEOMETRY = 4,
    ALIA_STYLE_GROUP_RECIPE = 5,
};

// name, description, and group shared by style descriptors
typedef struct alia_style_meta
{
    char const* name;
    char const* desc;
    uint8_t group;
} alia_style_meta;

// float knob descriptor - `index` selects `vars[i]` or a float field via
// `offsetof` / `sizeof(float)` in an all-float struct (seeds, recipe).
typedef struct alia_style_float_desc
{
    alia_style_meta meta;
    uint16_t index;
    float min;
    float max;
    float step;
    float default_light;
    float default_dark;
} alia_style_float_desc;

// palette color slot descriptor
typedef struct alia_style_color_desc
{
    alia_style_meta meta;
    uint8_t slot;
} alia_style_color_desc;

// Return `base[desc->index]`.
static inline float*
alia_style_float_at(float* base, alia_style_float_desc const* desc)
{
    return base + desc->index;
}

// Return `base[desc->index]`.
static inline float const*
alia_style_float_at_const(float const* base, alia_style_float_desc const* desc)
{
    return base + desc->index;
}

// Return style var descriptors for `ui` (core plus registered). Write the
// count to `*count` when `count` is not null.
alia_style_float_desc const*
alia_style_var_descs(alia_ui_system* ui, size_t* count);

// Return core style seed field descriptors. Write the count to `*count` when
// `count` is not null.
alia_style_float_desc const*
alia_style_seed_descs(size_t* count);

// Return palette recipe field descriptors. Write the count to `*count` when
// `count` is not null.
alia_style_float_desc const*
alia_style_recipe_descs(size_t* count);

// Return style color descriptors for `ui` (core plus registered). Write the
// count to `*count` when `count` is not null.
alia_style_color_desc const*
alia_style_color_descs(alia_ui_system* ui, size_t* count);

// Register an app style var on `ui`. `desc->index` must be in
// `[ALIA_STYLE_VAR_APP_BEGIN, ALIA_STYLE_VAR_COUNT)`. Return 0 on success.
int
alia_style_var_register(alia_ui_system* ui, alia_style_float_desc const* desc);

// Register an app style color on `ui`. `desc->slot` must be in
// `[ALIA_STYLE_COLOR_APP_BEGIN, ALIA_STYLE_COLOR_COUNT)`. Return 0 on success.
int
alia_style_color_register(
    alia_ui_system* ui, alia_style_color_desc const* desc);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_STYLING_DESCRIPTORS_H
