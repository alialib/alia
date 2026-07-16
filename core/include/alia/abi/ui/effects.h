#ifndef ALIA_ABI_UI_EFFECTS_H
#define ALIA_ABI_UI_EFFECTS_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>

ALIA_EXTERN_C_BEGIN

// EFFECTS
//
// Each effect is its own draw material (one program / PSO per material) so the
// bucket sorter groups instances by shader. Render backends allocate the
// material ID when the effect is registered.
//
// Draw commands carry an Alia surface-space region and an opaque param blob.
// How backends bind region / surface size / params to the GPU is
// renderer-specific (see each renderer's effect_register docs).
//
// Portable registration uses `alia_effect_desc` + `alia_ui_register_effect`
// (or `alia_renderer_ops.register_effect`). Native HLSL/GLSL source register
// APIs remain as a single-backend escape hatch.

typedef struct alia_ui_system alia_ui_system;

// Shader bytecode / source formats for portable effect registration.
typedef enum alia_shader_format
{
    // UTF-8 GLSL fragment source. Body-only: the GL backend prepends the
    // appropriate #version (GLSL ES 300 on Emscripten / WebGL, 330 core on
    // desktop GL). Prefer this over SPIR-V for the web product path.
    ALIA_SHADER_FORMAT_GLSL_ES = 1,
    // D3DCompile / fxc SM5 pixel-shader bytecode (DXBC).
    ALIA_SHADER_FORMAT_DXBC = 2,
} alia_shader_format;

typedef struct alia_shader_blob
{
    alia_shader_format format;
    void const* data;
    // Byte length of `data`. For GLSL_ES, this is the source length (NUL
    // terminator optional). For DXBC, the full bytecode size.
    size_t size;
} alia_shader_blob;

// Backend-neutral effect registration descriptor.
typedef struct alia_effect_desc
{
    alia_shader_blob shader;
    size_t params_size;
} alia_effect_desc;

// Register an effect via `ui->renderer.register_effect`. Returns 0 on
// success. Requires renderer ops already installed.
int
alia_ui_register_effect(
    alia_ui_system* ui,
    alia_effect_desc const* desc,
    alia_draw_material_id* out_material_id);

typedef struct alia_effect_style
{
    // Minimum layout size in logical pixels.
    alia_vec2f min_size;
} alia_effect_style;

typedef struct alia_effect_draw_command
{
    alia_draw_command base;
    // Allocated layout region in surface space (top-left origin).
    alia_box region;
    uint16_t params_size;
    // Points into the draw-pass arena; valid only during draw_bucket.
    void const* params;
} alia_effect_draw_command;

alia_effect_style const*
alia_default_effect_style(void);

// Record an effect draw for a known surface-space box.
// `material_id` is the ID returned when the effect was registered.
void
alia_draw_effect(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    alia_box region,
    void const* params,
    size_t params_size);

// Layout leaf that records an effect over its allocated box.
alia_element_id
alia_do_effect(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    void const* params,
    size_t params_size,
    alia_layout_flags_t layout_flags,
    alia_effect_style const* style);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_EFFECTS_H */
