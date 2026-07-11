#ifndef ALIA_RENDERERS_D3D11_RENDERER_H
#define ALIA_RENDERERS_D3D11_RENDERER_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/system/api.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

ALIA_EXTERN_C_BEGIN

typedef struct alia_d3d11_renderer alia_d3d11_renderer;

alia_struct_spec
alia_d3d11_renderer_object_spec(void);

alia_d3d11_renderer*
alia_d3d11_renderer_init(void* object_storage);

// Create GPU resources and register ALIA_PRIMITIVE_MATERIAL_ID.
// Device/context must outlive the renderer.
void
alia_d3d11_renderer_attach(
    alia_d3d11_renderer* renderer,
    alia_ui_system* ui,
    ID3D11Device* device,
    ID3D11DeviceContext* context);

void
alia_d3d11_renderer_upload_msdf_atlas(
    alia_d3d11_renderer* renderer, alia_msdf_atlas_image const* image);

void
alia_d3d11_renderer_destroy(alia_d3d11_renderer* renderer);

ALIA_EXTERN_C_END

#endif /* ALIA_RENDERERS_D3D11_RENDERER_H */
