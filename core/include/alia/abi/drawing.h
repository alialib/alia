#pragma once

#include <alia/abi/arena.h>
#include <alia/abi/base.h>
#include <alia/abi/geometry.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alia_draw_state
{
    alia_affine2 transform;
    alia_box clip;
    float opacity;
} alia_draw_state;

typedef uint32_t alia_draw_material_id;
typedef int32_t alia_z_index;

typedef struct alia_draw_command
{
    struct alia_draw_command* next;
    // TODO: Record state.
    // alia_draw_state state;
} alia_draw_command;

typedef struct alia_draw_bucket
{
    alia_draw_command* head;
    alia_draw_command* tail;
    uint32_t count;
    // TODO: Add generalized, material-specific summary info.
    uint32_t instance_count;
} alia_draw_bucket;

static inline void
alia_draw_bucket_append(alia_draw_bucket* bucket, alia_draw_command* cmd)
{
    cmd->next = NULL;
    if (!bucket->head)
        bucket->head = cmd;
    else
        bucket->tail->next = cmd;
    bucket->tail = cmd;
    bucket->count++;
}

typedef void (*alia_material_draw_fn)(
    void* user, alia_draw_bucket const* bucket);

typedef struct alia_material_vtable
{
    alia_material_draw_fn draw_bucket;
} alia_material_vtable;

typedef struct alia_draw_system alia_draw_system;

alia_draw_material_id
alia_register_material(
    alia_draw_system* system, alia_material_vtable vtable, void* user);

typedef struct alia_draw_bucket_table alia_draw_bucket_table;

typedef struct alia_draw_context
{
    alia_draw_system* system;
    alia_draw_bucket_table* buckets;
    alia_arena_view* arena;
} alia_draw_context;

alia_draw_bucket*
alia_get_draw_bucket(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id);

#ifdef __cplusplus
}
#endif
