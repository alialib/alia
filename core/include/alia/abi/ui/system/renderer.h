#ifndef ALIA_ABI_UI_SYSTEM_RENDERER_H
#define ALIA_ABI_UI_SYSTEM_RENDERER_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/msdf.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// Renderer-provided GPU hooks (GL, D3D11, etc.). The shell/UI may invoke
// these without knowing the concrete backend. Implementations are optional
// (null function = no-op).
typedef struct alia_renderer_ops
{
    void (*upload_msdf_atlas)(void* user, alia_msdf_atlas_image const* image);
    void* user;
} alia_renderer_ops;

void
alia_ui_system_set_renderer_ops(
    alia_ui_system* ui, alia_renderer_ops const* ops);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_SYSTEM_RENDERER_H */
