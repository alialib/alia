#ifndef ALIA_ABI_UI_GEOMETRY_H
#define ALIA_ABI_UI_GEOMETRY_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef uint32_t alia_clip_id;

typedef int32_t alia_z_index;

typedef struct alia_geometry_context
{
    // the current scale - logical pixels per physical pixel
    float scale;

    // the current offset in physical pixels
    alia_vec2f offset;

    // the current clipping region in physical pixels
    alia_box clip_region;
    // TODO: index of the current clipping region in the pass's clip array
    alia_clip_id clip_id;

    // the base z-index for the current context
    alia_z_index z_base;
} alia_geometry_context;

static inline float
alia_px(alia_context* ctx, float logical_px)
{
    return logical_px * ctx->geometry->scale;
}

static inline alia_box
alia_geometry_get_clip_region(alia_context* ctx)
{
    return ctx->geometry->clip_region;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_GEOMETRY_H */
