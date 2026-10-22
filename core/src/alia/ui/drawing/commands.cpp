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
    auto const key = make_bucket_key(
        alia_ctx_draw(ctx)->target_id,
        z_index,
        alia_ctx_geometry(ctx)->clip.id,
        material_id);
    alia_draw_bucket_table* const buckets = alia_ctx_draw(ctx)->buckets;
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
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size)
{
    auto* command = (alia_draw_command*) alia_arena_ptr(
        &alia_ctx_draw(ctx)->arena,
        alia_arena_alloc(&alia_ctx_draw(ctx)->arena, size));
    auto* bucket = find_or_create_draw_bucket(ctx, z_index, material_id);
    append_draw_command(bucket, command);
    return command;
}

} // extern "C"
