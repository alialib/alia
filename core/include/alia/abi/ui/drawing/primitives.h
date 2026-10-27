#ifndef ALIA_ABI_UI_DRAWING_PRIMITIVES_H
#define ALIA_ABI_UI_DRAWING_PRIMITIVES_H

#include <alia/abi/base/color.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/commands.h>
#include <alia/abi/ui/drawing/system.h>

ALIA_EXTERN_C_BEGIN

typedef uint16_t alia_primitive_type;

// built-in primitive types - Geometry lives in the low 8 bits. The packer ORs
// gradient flags into the instance type for the shaders.
enum
{
    ALIA_PRIMITIVE_BOX = 0,
    ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE = 1,
    ALIA_PRIMITIVE_SQUIRCLE = 2,
    ALIA_PRIMITIVE_MSDF_GLYPH = 3,

    ALIA_PRIMITIVE_TYPE_MASK = 255,
    // fill uses a gradient along `gradient_angle`
    ALIA_PRIMITIVE_FLAG_GRADIENT_FILL = 256,
    // border uses the same gradient - The packer sets this when the fill is a
    // gradient, the border width is positive, and the border stops differ.
    ALIA_PRIMITIVE_FLAG_GRADIENT_BORDER = 512,
};

// edge and gradient style for an analytic shape
typedef struct alia_shape_style
{
    float border_width;

    // when true, fill (and possibly border) use `gradient_angle`
    bool gradient;

    // gradient direction in shape-local degrees - Zero points to the right,
    // and positive angles run clockwise. Unused when `gradient` is false.
    float gradient_angle;

    // edge falloff in physical pixels - Zero keeps the 1px antialiased edge.
    float sigma;

    alia_srgba8 border_color;
    alia_srgba8 fill_end_color;
    // second border stop - Equal to `border_color` for a solid stroke.
    alia_srgba8 border_end_color;
} alia_shape_style;

typedef struct alia_draw_box_payload
{
    float corner_radius;
    // fill and edge style
    alia_shape_style style;
} alia_draw_box_payload;

typedef struct alia_draw_equilateral_triangle_payload
{
    // degrees, positive rotates clockwise in screen space
    float rotation_degrees;
    // fill and edge style
    alia_shape_style style;
} alia_draw_equilateral_triangle_payload;

typedef struct alia_draw_squircle_payload
{
    float radius;
    // fill and edge style
    alia_shape_style style;
} alia_draw_squircle_payload;

typedef struct alia_draw_msdf_glyph_payload
{
    float uv_rect[4];
    float sdf_scale;
} alia_draw_msdf_glyph_payload;

typedef union alia_primitive_payload
{
    alia_draw_box_payload box;
    alia_draw_equilateral_triangle_payload triangle;
    alia_draw_squircle_payload squircle;
    alia_draw_msdf_glyph_payload msdf_glyph;
} alia_primitive_payload;

typedef struct alia_draw_primitive_command
{
    alia_draw_command base;
    alia_box box;
    alia_primitive_type primitive_type;
    // general per-primitive color (usually the fill color)
    alia_srgba8 color;
    // union-style per-primitive payload (depends on `primitive_type`)
    alia_primitive_payload payload;
} alia_draw_primitive_command;

typedef struct alia_box_paint
{
    alia_srgba8 fill_color;
    float corner_radius;
    float border_width;
    alia_srgba8 border_color;
    alia_srgba8 fill_end_color;
    alia_srgba8 border_end_color;
    // See `alia_shape_style::gradient`.
    bool gradient = false;
    // See `alia_shape_style::gradient_angle`.
    float gradient_angle = 0.f;
    // See `alia_shape_style::sigma`.
    float sigma = 0.f;
} alia_box_paint;

static inline alia_shape_style
alia_shape_style_solid(void)
{
    alia_shape_style style;
    style.border_width = 0.f;
    style.gradient = false;
    style.gradient_angle = 0.f;
    style.sigma = 0.f;
    style.border_color = alia_srgba8{0, 0, 0, 0};
    style.fill_end_color = alia_srgba8{0, 0, 0, 0};
    style.border_end_color = alia_srgba8{0, 0, 0, 0};
    return style;
}

static inline alia_shape_style
alia_shape_style_from_box_paint(alia_box_paint paint)
{
    alia_shape_style style;
    style.border_width = paint.border_width;
    style.gradient = paint.gradient;
    style.gradient_angle = paint.gradient_angle;
    style.sigma = paint.sigma;
    style.border_color = paint.border_color;
    style.fill_end_color = paint.fill_end_color;
    style.border_end_color = paint.border_end_color;
    return style;
}

static inline alia_draw_primitive_command*
alia_draw_shape_alloc(
    alia_ui_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_primitive_type primitive_type,
    alia_srgba8 fill)
{
    alia_draw_primitive_command* command
        = (alia_draw_primitive_command*) alia_draw_command_alloc(
            ctx,
            z_index,
            ALIA_PRIMITIVE_MATERIAL_ID,
            ALIA_MIN_ALIGNED_SIZE(sizeof(alia_draw_primitive_command)));

    command->box = alia_box_translate(box, alia_ctx_geometry(ctx)->offset);
    command->primitive_type = primitive_type;
    command->color = fill;
    return command;
}

static inline void
alia_draw_box(
    alia_ui_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_box_paint paint)
{
    alia_draw_primitive_command* command = alia_draw_shape_alloc(
        ctx, z_index, box, ALIA_PRIMITIVE_BOX, paint.fill_color);
    command->payload.box.corner_radius = paint.corner_radius;
    command->payload.box.style = alia_shape_style_from_box_paint(paint);
}

static inline void
alia_draw_rounded_box(
    alia_ui_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_srgba8 color,
    float radius)
{
    alia_draw_box(
        ctx,
        z_index,
        box,
        {.fill_color = color,
         .corner_radius = radius,
         .border_width = 0,
         .border_color = color});
}

static inline void
alia_draw_equilateral_triangle(
    alia_ui_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_srgba8 color,
    float rotation_degrees,
    alia_shape_style style = alia_shape_style_solid())
{
    alia_draw_primitive_command* command = alia_draw_shape_alloc(
        ctx, z_index, box, ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE, color);
    command->payload.triangle.rotation_degrees = rotation_degrees;
    command->payload.triangle.style = style;
}

static inline void
alia_draw_circle(
    alia_ui_context* ctx,
    alia_z_index z_index,
    alia_vec2f center,
    float radius,
    alia_srgba8 color,
    float sigma = 0.f)
{
    alia_draw_box(
        ctx,
        z_index,
        alia_box{
            alia_vec2f_sub(center, alia_vec2f{radius, radius}),
            alia_vec2f{2 * radius, 2 * radius}},
        {.fill_color = color,
         .corner_radius = radius,
         .border_width = 0,
         .border_color = color,
         .sigma = sigma});
}

static inline void
alia_draw_squircle(
    alia_ui_context* ctx,
    alia_z_index z_index,
    alia_box box,
    float radius,
    alia_srgba8 color,
    alia_shape_style style = alia_shape_style_solid())
{
    alia_draw_primitive_command* command = alia_draw_shape_alloc(
        ctx, z_index, box, ALIA_PRIMITIVE_SQUIRCLE, color);
    command->payload.squircle.radius = radius;
    command->payload.squircle.style = style;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_PRIMITIVES_H */
