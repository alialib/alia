#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/commands.h>
#include <alia/abi/ui/drawing/system.h>
#include <alia/abi/ui/geometry/api.h>

#include <alia/impl/context.hpp>
#include <alia/ui/drawing/bucket_key.h>
#include <alia/ui/drawing/system.h>
#include <alia/ui/system/object.h>

namespace {

static alia_draw_bucket*
find_or_create_draw_bucket(
    alia_ui_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id)
{
    alia_draw_context* const draw = alia_ctx_draw(ctx);
    auto const key = make_bucket_key(
        draw->target_id,
        z_index,
        alia_ctx_geometry(ctx)->clip.id,
        material_id);
    alia_draw_bucket_table* const buckets = draw->buckets;
    auto it = buckets->buckets.find(key);
    if (it == buckets->buckets.end())
    {
        buckets->keys.push_back(key);
        it = buckets->buckets.insert(
            it,
            {key,
             alia_draw_bucket{
                 .clip_rect = alia_ctx_geometry(ctx)->clip.pointer,
                 .head = nullptr,
                 .tail = nullptr,
                 .count = 0,
                 .instance_count = 0,
             }});
    }
    return &it->second;
}

static inline void
append_draw_command(alia_draw_bucket* bucket, alia_draw_command* cmd)
{
    cmd->next = NULL;
    if (!bucket->head)
        bucket->head = cmd;
    else
        bucket->tail->next = cmd;
    bucket->tail = cmd;
    bucket->count++;
}

} // namespace

extern "C" {

alia_draw_command*
alia_draw_command_alloc(
    alia_ui_context* ctx,
    alia_draw_id id,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size)
{
    alia_draw_context* const draw = alia_ctx_draw(ctx);
    auto* command = (alia_draw_command*) alia_arena_ptr(
        &draw->arena, alia_arena_alloc(&draw->arena, size));
    command->id = id.id;
    command->sub_id = id.sub_id;
    command->frame = draw->current_frame;
    auto* bucket = find_or_create_draw_bucket(ctx, z_index, material_id);
    append_draw_command(bucket, command);
    return command;
}

void
alia_draw_push_frame(alia_ui_context* ctx, alia_vec2f translation)
{
    alia_draw_context* const draw = alia_ctx_draw(ctx);
    if (!draw || !draw->frames)
        return;

    alia_draw_frame_table* const table = draw->frames;
    alia_frame_id const parent = draw->current_frame;
    alia_vec2f absolute = translation;
    if (parent != ALIA_FRAME_ID_NONE)
    {
        ALIA_ASSERT(parent < table->entries.size());
        absolute
            = alia_vec2f_add(table->entries[parent].absolute, translation);
    }

    alia_frame_id const id = static_cast<alia_frame_id>(table->entries.size());
    ALIA_ASSERT(id != ALIA_FRAME_ID_NONE);
    table->entries.push_back(
        alia_draw_frame{
            .parent = parent,
            .translation = translation,
            .absolute = absolute,
        });
    draw->current_frame = id;
}

void
alia_draw_pop_frame(alia_ui_context* ctx)
{
    alia_draw_context* const draw = alia_ctx_draw(ctx);
    if (!draw || !draw->frames)
        return;

    alia_draw_frame_table* const table = draw->frames;
    ALIA_ASSERT(draw->current_frame < table->entries.size());
    alia_frame_id const parent = table->entries[draw->current_frame].parent;
    ALIA_ASSERT(parent != ALIA_FRAME_ID_NONE);
    draw->current_frame = parent;
}

alia_frame_id
alia_draw_begin_root_frame(alia_ui_context* ctx)
{
    alia_draw_context* const draw = alia_ctx_draw(ctx);
    ALIA_ASSERT(draw && draw->frames);
    alia_frame_id const previous = draw->current_frame;

    alia_draw_frame_table* const table = draw->frames;
    alia_frame_id const id = static_cast<alia_frame_id>(table->entries.size());
    ALIA_ASSERT(id != ALIA_FRAME_ID_NONE);
    table->entries.push_back(
        alia_draw_frame{
            .parent = ALIA_FRAME_ID_NONE,
            .translation = {0.f, 0.f},
            .absolute = {0.f, 0.f},
        });
    draw->current_frame = id;
    return previous;
}

void
alia_draw_restore_frame(alia_ui_context* ctx, alia_frame_id frame)
{
    alia_draw_context* const draw = alia_ctx_draw(ctx);
    if (!draw || !draw->frames)
        return;
    ALIA_ASSERT(
        frame == ALIA_FRAME_ID_NONE || frame < draw->frames->entries.size());
    draw->current_frame = frame;
}

} // extern "C"
