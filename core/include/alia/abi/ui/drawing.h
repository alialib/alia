#ifndef ALIA_ABI_UI_DRAWING_H
#define ALIA_ABI_UI_DRAWING_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/geometry.h>

ALIA_EXTERN_C_BEGIN

// LIFECYCLE

typedef struct alia_draw_system alia_draw_system;

alia_struct_spec
alia_draw_system_object_spec(void);

alia_draw_system*
alia_draw_system_init(void* object_storage, alia_vec2f surface_size);

void
alia_draw_system_cleanup(alia_draw_system* system);

// MATERIALS

typedef uint32_t alia_draw_material_id;

// built-in material IDs
enum
{
    ALIA_BOX_MATERIAL_ID = 0,
    // TODO: monospaced/variable-width debug text material IDs
    ALIA_BUILTIN_MATERIAL_COUNT = 1,
};

// Allocate IDs for custom materials.
alia_draw_material_id
alia_material_alloc_ids(alia_draw_system* system, uint32_t count);

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
    alia_draw_system* system,
    alia_draw_material_id id,
    alia_material_vtable vtable,
    void* user);

// GENERIC DRAW COMMANDS

typedef struct alia_draw_bucket_table alia_draw_bucket_table;

typedef struct alia_draw_context
{
    alia_draw_system* system;
    alia_draw_bucket_table* buckets;
    alia_bump_allocator arena;
} alia_draw_context;

// Allocate a draw command with `ALIA_MIN_ALIGN` alignment.
alia_draw_command*
alia_draw_command_alloc(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size);

// Allocate a draw command with a custom alignment.
// `alignment` must be a power of two and no greater than `ALIA_MAX_ALIGN`.
// `size` must be a multiple of `ALIA_MIN_ALIGN`.
alia_draw_command*
alia_draw_command_alloc_aligned(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size,
    size_t alignment);

// BOXES

typedef struct alia_box_paint
{
    alia_rgba fill_color;
    float corner_radius;
    float border_width;
    alia_rgba border_color;
} alia_box_paint;

typedef struct alia_draw_box_command
{
    alia_draw_command base;
    alia_box box;
    alia_box_paint paint;
} alia_draw_box_command;

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
        alia_draw_box_command* command
            = (alia_draw_box_command*) alia_draw_command_alloc(
                ctx->draw,
                z_index,
                ALIA_BOX_MATERIAL_ID,
                ALIA_MIN_ALIGNED_SIZE(sizeof(alia_draw_box_command)));

        command->box = box;
        if (paint.border_width == 0)
            paint.border_color = paint.fill_color;
        command->paint = paint;
    }
}

static inline void
alia_draw_rounded_box(
    alia_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_rgba color,
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
alia_draw_circle(
    alia_context* ctx,
    alia_z_index z_index,
    alia_vec2f center,
    float radius,
    alia_rgba color)
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

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_H */
