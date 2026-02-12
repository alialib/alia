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
    // the transformation from the active frame of reference to surface space
    alia_affine2 transform;

    // the current clipping region
    alia_box clip_region;
    // TODO: index of the current clipping region in the pass's clip array
    alia_clip_id clip_id;

    // the base z-index for the current context
    alia_z_index z_base;
} alia_geometry_context;

static inline alia_affine2
alia_geometry_get_transform(alia_context* ctx)
{
    return ctx->geometry->transform;
}

static inline alia_box
alia_geometry_get_clip_region(alia_context* ctx)
{
    return ctx->geometry->clip_region;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_GEOMETRY_H */
