#ifndef ALIA_ABI_UI_GEOMETRY_TYPES_H
#define ALIA_ABI_UI_GEOMETRY_TYPES_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef uint16_t alia_clip_id;

typedef uint16_t alia_z_index;

// Z indices are organized as layers of planes.
// This defines the number of planes within a layer.
enum
{
    ALIA_Z_LAYER_SPACING = 16
};

// The following describes how the planes within a layer are organized.
#define ALIA_Z_PLANES(X)                                                      \
    X(0x0, SHADOW, "drop shadows and underlays")                              \
    X(0x1, BACKGROUND, "fills and ordinary chrome")                           \
    X(0x4, CONTENT, "text, icons, interactive elements")                      \
    X(0x8, ACCENT, "focus/hover accents above content")                       \
    X(0xc, EFFECT, "transient effects such as click flares")

// the default z-base at the start of a UI pass
enum
{
    ALIA_DEFAULT_Z_BASE = 256,
};

#define ALIA_DEFINE_Z_PLANE(offset, name, description)                        \
    ALIA_Z_PLANE_##name = (offset),
enum
{
    ALIA_Z_PLANES(ALIA_DEFINE_Z_PLANE)
};
#undef ALIA_DEFINE_Z_PLANE

typedef struct alia_clip_state
{
    // the value of the current clipping rectangle in logical pixels
    // This is duplicated here for faster access.
    alia_box box;
    // the pointer to the clipping rectangle in physical pixels
    // This is guaranteed to be valid for the full pass.
    alia_box* pointer;
    // a unique numeric ID for the current clipping rectangle
    // This is unique for the current pass.
    alia_clip_id id;
} alia_clip_state;

typedef struct alia_geometry_context
{
    // current logical-to-physical scale (physical = logical * scale)
    float scale;

    // the current offset in physical pixels
    alia_vec2f offset;

    // the current clipping state
    alia_clip_state clip;

    // the next clip ID to use
    alia_clip_id next_clip_id;

    // base z-index of the current layer
    alia_z_index z_base;
} alia_geometry_context;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_GEOMETRY_TYPES_H */
