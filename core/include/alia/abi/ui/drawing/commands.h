#ifndef ALIA_ABI_UI_DRAWING_COMMANDS_H
#define ALIA_ABI_UI_DRAWING_COMMANDS_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/geometry.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_context alia_context;

// IDs used when recording and bucketing draw commands
typedef uint16_t alia_draw_material_id;
typedef uint16_t alia_draw_target_id;

enum
{
    // the base UI surface (i.e., the window)
    ALIA_DRAW_TARGET_PRIMARY = 0,
};

typedef struct alia_draw_command
{
    struct alia_draw_command* next;
} alia_draw_command;

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
    // active draw target
    alia_draw_target_id target_id;
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

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_COMMANDS_H */
