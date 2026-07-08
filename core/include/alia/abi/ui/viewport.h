#ifndef ALIA_ABI_UI_VIEWPORT_H
#define ALIA_ABI_UI_VIEWPORT_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_viewport_style
{
    // Minimum layout size in logical pixels.
    alia_vec2f min_size;
} alia_viewport_style;

// Standard payload for custom viewport materials.
typedef struct alia_viewport_draw_command
{
    alia_draw_command base;
    // Allocated layout region in surface space (top-left origin).
    alia_box region;
    // Scene-to-surface transform. Identity when no camera is configured.
    alia_affine2 scene_to_surface;
} alia_viewport_draw_command;

alia_viewport_style const*
alia_default_viewport_style(void);

alia_element_id
alia_do_viewport(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    alia_layout_flags_t layout_flags,
    alia_viewport_style const* style);

// OpenGL viewport rectangle in device pixels (bottom-left origin).
typedef struct alia_gl_viewport
{
    int x;
    int y;
    int width;
    int height;
} alia_gl_viewport;

// Map a surface-space viewport region to an OpenGL viewport rectangle.
alia_gl_viewport
alia_viewport_region_to_gl_viewport(alia_box region, alia_vec2f surface_size);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_VIEWPORT_H */
