#ifndef ALIA_RENDERERS_GL_RENDERER_H
#define ALIA_RENDERERS_GL_RENDERER_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/system/api.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_gl_renderer alia_gl_renderer;

alia_struct_spec
alia_gl_renderer_object_spec(void);

// Zero-initialize renderer storage. Does not touch GL; call
// `alia_gl_renderer_attach` once a context is current.
alia_gl_renderer*
alia_gl_renderer_init(void* object_storage);

// Create GPU resources and register the built-in primitive material.
// Requires an active GL context.
void
alia_gl_renderer_attach(alia_gl_renderer* renderer, alia_ui_system* ui);

void
alia_gl_renderer_upload_msdf_atlas(
    alia_gl_renderer* renderer, alia_msdf_atlas_image const* image);

// Release GPU resources owned by the renderer. Does not free `object_storage`.
void
alia_gl_renderer_destroy(alia_gl_renderer* renderer);

ALIA_EXTERN_C_END

#endif /* ALIA_RENDERERS_GL_RENDERER_H */
