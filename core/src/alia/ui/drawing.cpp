#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>

#include <alia/abi/ui/events.h>
#include <alia/context.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/prelude.hpp>
#include <alia/ui/drawing.h>
#include <alia/ui/system/object.h>

#include <algorithm>

extern "C" {

alia_draw_material_id
alia_material_alloc_ids(alia_ui_system* system, uint32_t count)
{
    if (system->draw.next_material_id < ALIA_BUILTIN_MATERIAL_COUNT)
        system->draw.next_material_id = ALIA_BUILTIN_MATERIAL_COUNT;
    auto const start_id = system->draw.next_material_id;
    system->draw.next_material_id += count;
    return start_id;
}

void
alia_material_register(
    alia_ui_system* system,
    alia_draw_material_id id,
    alia_material_vtable vtable,
    void* user)
{
    if (id >= system->draw.materials.size())
        system->draw.materials.resize(size_t(id) + 1);
    system->draw.materials[id] = {.vtable = vtable, .user = user};
}

void
alia_ui_execute_draw_pass(alia_ui_system* system)
{
    ALIA_ASSERT(system);

    alia_draw_bucket_table bucket_table = {
        .buckets = {},
        .keys = {},
    };

    alia_draw_context draw_context = {
        .buckets = &bucket_table,
        .arena = {},
    };
    alia_bump_allocator_init(&draw_context.arena, &system->draw.command_arena);

    auto draw_event = alia_make_draw_event({.context = &draw_context});
    alia::dispatch_event(*system, draw_event);

    alia_bump_allocator_commit_peak(&draw_context.arena);

    std::sort(bucket_table.keys.begin(), bucket_table.keys.end());
    for (auto const key : bucket_table.keys)
    {
        alia_draw_bucket* bucket = &bucket_table.buckets[key];
        alia_draw_material_id const material_id = key & 0xffff;
        alia_draw_material* material = &system->draw.materials[material_id];
        material->vtable.draw_bucket(material->user, bucket);
    }
}

inline uint64_t
make_bucket_key(
    alia_z_index z_index,
    alia_clip_id clip_id,
    alia_draw_material_id material_id)
{
    return (uint64_t(uint32_t(z_index)) << 32) | (uint64_t(clip_id) << 16)
         | uint64_t(material_id);
}

static alia_draw_bucket*
find_or_create_draw_bucket(
    alia_context* ctx, alia_z_index z_index, alia_draw_material_id material_id)
{
    auto const key
        = make_bucket_key(z_index, ctx->geometry->clip.id, material_id);
    alia_draw_bucket_table* const buckets = ctx->draw->buckets;
    auto it = buckets->buckets.find(key);
    if (it == buckets->buckets.end())
    {
        buckets->keys.push_back(key);
        it = buckets->buckets.insert(
            it,
            {key,
             alia_draw_bucket{
                 .clip_rect = ctx->geometry->clip.pointer,
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

alia_draw_command*
alia_draw_command_alloc(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size)
{
    auto* command = (alia_draw_command*) alia_arena_ptr(
        &ctx->draw->arena, alia_arena_alloc(&ctx->draw->arena, size));
    auto* bucket = find_or_create_draw_bucket(ctx, z_index, material_id);
    append_draw_command(bucket, command);
    return command;
}

} // extern "C"
