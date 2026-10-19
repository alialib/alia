#ifndef ALIA_ABI_UI_GEOMETRY_API_H
#define ALIA_ABI_UI_GEOMETRY_API_H

#include <alia/abi/ui/context.h>
#include <alia/abi/ui/geometry/types.h>

ALIA_EXTERN_C_BEGIN

void
alia_geometry_push_clip_box(alia_ui_context* ctx, alia_box box);

void
alia_geometry_pop_clip_box(alia_ui_context* ctx);

void
alia_geometry_push_translation(alia_ui_context* ctx, alia_vec2f offset);

void
alia_geometry_pop_translation(alia_ui_context* ctx);

// Multiply the current geometry scale by `factor` for a nested region.
void
alia_geometry_push_scale(alia_ui_context* ctx, float factor);

void
alia_geometry_pop_scale(alia_ui_context* ctx);

// Push a new Z base index.
void
alia_geometry_push_z_base(alia_ui_context* ctx, alia_z_index z_base);

// Push a new Z base index, increased by `delta` above the current Z base.
void
alia_geometry_push_z_offset(alia_ui_context* ctx, alia_z_index delta);

void
alia_geometry_pop_z(alia_ui_context* ctx);

static inline float
alia_px(alia_ui_context* ctx, float logical_px)
{
    return logical_px * alia_ctx_geometry(ctx)->scale;
}

// Get the absolute Z index for `plane` in the current layer.
static inline alia_z_index
alia_z(alia_ui_context* ctx, alia_z_index plane)
{
    return (alia_z_index) (alia_ctx_geometry(ctx)->z_base + plane);
}

static inline alia_box
alia_geometry_get_clip_box(alia_ui_context* ctx)
{
    return alia_ctx_geometry(ctx)->clip.box;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_GEOMETRY_API_H */
