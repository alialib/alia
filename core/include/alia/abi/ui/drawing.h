#ifndef ALIA_ABI_UI_DRAWING_H
#define ALIA_ABI_UI_DRAWING_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/geometry.h>

ALIA_EXTERN_C_BEGIN

// MATERIALS

typedef uint16_t alia_draw_material_id;

// built-in material IDs
enum
{
    ALIA_PRIMITIVE_MATERIAL_ID = 0,
    // TODO: monospaced/variable-width debug text material IDs
    ALIA_BUILTIN_MATERIAL_COUNT = 1,
};

// Allocate IDs for custom materials.
alia_draw_material_id
alia_material_alloc_ids(alia_ui_system* system, uint32_t count);

typedef struct alia_draw_command
{
    struct alia_draw_command* next;
} alia_draw_command;

typedef struct alia_draw_bucket alia_draw_bucket;

typedef void (*alia_material_draw_fn)(
    void* user, alia_draw_bucket const* bucket);

typedef struct alia_material_vtable
{
    alia_material_draw_fn draw_bucket;
} alia_material_vtable;

// Register the vtable for a material.
// The ID can either be a built-in ID or
// a custom ID allocated with `alia_material_alloc_ids()`.
void
alia_material_register(
    alia_ui_system* system,
    alia_draw_material_id id,
    alia_material_vtable vtable,
    void* user);

// GENERIC DRAW COMMANDS

typedef struct alia_draw_bucket
{
    alia_box* clip_rect;
    alia_draw_command* head;
    alia_draw_command* tail;
    uint32_t count;
    // TODO: Add generalized, material-specific summary info.
    uint32_t instance_count;
} alia_draw_bucket;

typedef struct alia_draw_bucket_table alia_draw_bucket_table;

typedef struct alia_draw_context
{
    alia_draw_bucket_table* buckets;
    alia_bump_allocator arena;
} alia_draw_context;

// Allocate a draw command with `ALIA_MIN_ALIGN` alignment.
alia_draw_command*
alia_draw_command_alloc(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size);

// Allocate a draw command with a custom alignment.
// `alignment` must be a power of two and no greater than `ALIA_MAX_ALIGN`.
// `size` must be a multiple of `ALIA_MIN_ALIGN`.
alia_draw_command*
alia_draw_command_alloc_aligned(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size,
    size_t alignment);

// PRIMITIVES

typedef uint16_t alia_primitive_type;

// built-in primitive types
enum
{
    ALIA_PRIMITIVE_BOX = 0,
    ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE = 1,
    ALIA_PRIMITIVE_SQUIRCLE = 2,
};

typedef struct alia_draw_box_payload
{
    float corner_radius;
    float border_width;
    alia_srgba8 border_color;
} alia_draw_box_payload;

typedef struct alia_draw_equilateral_triangle_payload
{
    // degrees, positive rotates clockwise in screen space
    float rotation_degrees;
} alia_draw_equilateral_triangle_payload;

typedef struct alia_draw_squircle_payload
{
    float radius;
    float border_width;
    alia_srgba8 border_color;
} alia_draw_squircle_payload;

typedef union alia_primitive_payload
{
    alia_draw_box_payload box;
    alia_draw_equilateral_triangle_payload triangle;
    alia_draw_squircle_payload squircle;
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

// BOXES

// TODO: Revisit this structure breakdown.
typedef struct alia_box_paint
{
    alia_srgba8 fill_color;
    float corner_radius;
    float border_width;
    alia_srgba8 border_color;
} alia_box_paint;

// TODO: Provide specific overloads for drawing filled/outlined
// rounded/non-rounded boxes.

static inline void
alia_draw_box(
    alia_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_box_paint paint)
{
    {
        if (paint.border_width == 0)
            paint.border_color = paint.fill_color;

        alia_draw_primitive_command* command
            = (alia_draw_primitive_command*) alia_draw_command_alloc(
                ctx,
                z_index,
                ALIA_PRIMITIVE_MATERIAL_ID,
                ALIA_MIN_ALIGNED_SIZE(sizeof(alia_draw_primitive_command)));

        command->box = alia_box_translate(box, ctx->geometry->offset);
        command->primitive_type = ALIA_PRIMITIVE_BOX;
        command->color = paint.fill_color;
        command->payload.box.corner_radius = paint.corner_radius;
        command->payload.box.border_width = paint.border_width;
        command->payload.box.border_color = paint.border_color;
    }
}

static inline void
alia_draw_rounded_box(
    alia_context* ctx,
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
    alia_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_srgba8 color,
    float rotation_degrees)
{
    alia_draw_primitive_command* command
        = (alia_draw_primitive_command*) alia_draw_command_alloc(
            ctx,
            z_index,
            ALIA_PRIMITIVE_MATERIAL_ID,
            ALIA_MIN_ALIGNED_SIZE(sizeof(alia_draw_primitive_command)));

    command->box = alia_box_translate(box, ctx->geometry->offset);
    command->primitive_type = ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE;
    command->color = color;
    command->payload.triangle.rotation_degrees = rotation_degrees;
}

static inline void
alia_draw_circle(
    alia_context* ctx,
    alia_z_index z_index,
    alia_vec2f center,
    float radius,
    alia_srgba8 color)
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
         .border_color = color});
}

static inline void
alia_draw_squircle(
    alia_context* ctx,
    alia_z_index z_index,
    alia_box box,
    float radius,
    alia_srgba8 color)
{
    alia_draw_primitive_command* command
        = (alia_draw_primitive_command*) alia_draw_command_alloc(
            ctx,
            z_index,
            ALIA_PRIMITIVE_MATERIAL_ID,
            ALIA_MIN_ALIGNED_SIZE(sizeof(alia_draw_primitive_command)));

    command->box = alia_box_translate(box, ctx->geometry->offset);
    command->primitive_type = ALIA_PRIMITIVE_SQUIRCLE;
    command->color = color;
    command->payload.squircle.radius = radius;
    command->payload.squircle.border_width = 0;
    command->payload.squircle.border_color = color;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_H */
