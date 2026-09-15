#ifndef ALIA_ABI_UI_DRAWING_SHADER_H
#define ALIA_ABI_UI_DRAWING_SHADER_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/commands.h>
#include <alia/abi/ui/drawing/system.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>

ALIA_EXTERN_C_BEGIN

// This file defines a custom shader API for rendering lightweight procedural
// graphics on a UI surface.
//
// Shader programs cover a rectangular region of the surface and can accept an
// arbitrary (but fixed) number of uniform parameters that are passed in by the
// component code.
//
//
// Each registered shader is its own draw material. Renderers are in charge of
// allocating the material ID when the shader is registered. They also handle
// binding the region and parameters to the GPU.
//
// The core API provides a unified shader registration API that accepts tagged
// shader bytecode. However, Alia doesn't impose any specific format on this
// bytecode. It's up to the app and the renderer to agree on a format. In
// multi-platform applications, the intention is that this would be integrated
// with a higher-level shader language and a build pipeline that compiles to an
// appropriate bytecode format. (Renderers are also free to provide their own
// native registration APIs if that is more convenient for single-platform
// applications.)

typedef struct alia_ui_system alia_ui_system;

// The shader format is a FourCC identifying the bytecode/source kind.
// (See ALIA_FOURCC for construction.) These are opaque to the core.
typedef uint32_t alia_shader_format;

typedef struct alia_shader_blob
{
    alia_shader_format format;
    void const* data;
    size_t size;
} alia_shader_blob;

typedef struct alia_shader_desc
{
    alia_shader_blob shader;
    size_t params_size;
} alia_shader_desc;

// Register a shader. Returns 0 on success.
int
alia_ui_register_shader(
    alia_ui_system* ui,
    alia_shader_desc const* desc,
    alia_draw_material_id* out_material_id);

typedef struct alia_shader_draw_command
{
    alia_draw_command base;
    // allocated layout region in surface space (top-left origin)
    alia_box region;
    uint16_t params_size;
    // points into the draw-pass arena; valid only during draw_bucket
    void const* params;
} alia_shader_draw_command;

// Record a shader draw command.
void
alia_draw_shader(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    alia_box region,
    void const* params,
    size_t params_size);

// "Do" a shader component that participates in layout.
// TODO: Move elsewhere?
alia_element_id
alia_do_shader(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    void const* params,
    size_t params_size,
    alia_layout_flags_t layout_flags,
    alia_vec2f min_size);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_SHADER_H */
