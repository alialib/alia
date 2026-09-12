#ifndef ALIA_ABI_UI_GEOMETRY_H
#define ALIA_ABI_UI_GEOMETRY_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef uint16_t alia_clip_id;

typedef uint16_t alia_z_index;

enum
{
    // default `z_base` for a top-level pass
    ALIA_DEFAULT_Z_BASE = 256,
    // Z spacing between UI layers
    ALIA_Z_LAYER_SPACING = 4,
};

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
    // the current scale - logical pixels per physical pixel
    float scale;

    // the current offset in physical pixels
    alia_vec2f offset;

    // the current clipping state
    alia_clip_state clip;

    // the next clip ID to use
    alia_clip_id next_clip_id;

    // the base z-index for the current context
    alia_z_index z_base;
} alia_geometry_context;

static inline float
alia_px(alia_context* ctx, float logical_px)
{
    return logical_px * ctx->geometry->scale;
}

static inline alia_box
alia_geometry_get_clip_box(alia_context* ctx)
{
    return ctx->geometry->clip.box;
}

void
alia_geometry_push_clip_box(alia_context* ctx, alia_box box);

void
alia_geometry_pop_clip_box(alia_context* ctx);

void
alia_geometry_push_translation(alia_context* ctx, alia_vec2f offset);

void
alia_geometry_pop_translation(alia_context* ctx);

// Push a new Z base index.
void
alia_geometry_push_z_base(alia_context* ctx, alia_z_index z_base);

// Push a new Z base index, increased by `delta` above the current Z base.
void
alia_geometry_push_z_offset(alia_context* ctx, alia_z_index delta);

void
alia_geometry_pop_z(alia_context* ctx);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_GEOMETRY_H */
