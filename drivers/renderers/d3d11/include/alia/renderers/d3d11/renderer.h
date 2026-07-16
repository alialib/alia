#ifndef ALIA_RENDERERS_D3D11_RENDERER_H
#define ALIA_RENDERERS_D3D11_RENDERER_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/effects.h>
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

// Native-source hatch: compile HLSL to DXBC and register. Prefer
// `alia_ui_register_effect` with `ALIA_SHADER_FORMAT_DXBC` for portable
// registration. The pixel shader should declare:
//   cbuffer AliaEffectFrame : register(b0) {
//     float4 alia_effect_region;  // xy=min, zw=size (Alia surface space)
//     float4 alia_effect_surface; // xy=surface size in pixels
//   };
//   cbuffer Effect : register(b1) { /* user params, params_size bytes */ };
typedef struct alia_d3d11_effect_desc
{
    char const* pixel_shader_hlsl;
    char const* entry_point; // optional; defaults to "ps_main"
    size_t params_size;
} alia_d3d11_effect_desc;

int
alia_d3d11_effect_register(
    alia_d3d11_renderer* renderer,
    alia_d3d11_effect_desc const* desc,
    alia_draw_material_id* out_material_id);

void
alia_d3d11_renderer_destroy(alia_d3d11_renderer* renderer);

ALIA_EXTERN_C_END

#endif /* ALIA_RENDERERS_D3D11_RENDERER_H */
