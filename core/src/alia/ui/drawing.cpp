#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>

#include <alia/arena.hpp>
#include <alia/context.hpp>
#include <alia/prelude.hpp>
#include <alia/ui/drawing.h>

extern "C" {

alia_draw_material_id
alia_material_alloc_ids(alia_draw_system* system, uint32_t count)
{
    if (system->next_material_id < ALIA_BUILTIN_MATERIAL_COUNT)
        system->next_material_id = ALIA_BUILTIN_MATERIAL_COUNT;
    auto const start_id = system->next_material_id;
    system->next_material_id += count;
    return start_id;
}

void
alia_material_register(
    alia_draw_system* system,
    alia_draw_material_id id,
    alia_material_vtable vtable,
    void* user)
{
    if (id >= system->materials.size())
        system->materials.resize(size_t(id) + 1);
    system->materials[id] = {.vtable = vtable, .user = user};
}

inline uint64_t
make_bucket_key(alia_z_index z_index, alia_draw_material_id material_id)
{
    return (uint64_t(uint32_t(z_index)) << 32) | uint64_t(material_id);
}

static alia_draw_bucket*
find_or_create_draw_bucket(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id)
{
    auto const key = make_bucket_key(z_index, material_id);
    auto it = ctx->buckets->buckets.find(key);
    if (it == ctx->buckets->buckets.end())
    {
        ctx->buckets->keys.push_back(key);
        it = ctx->buckets->buckets.insert(
            it,
            {key,
             alia_draw_bucket{
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
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size)
{
    auto* command = (alia_draw_command*) alia_arena_ptr(
        ctx->arena, alia_arena_alloc(ctx->arena, size));
    auto* bucket = find_or_create_draw_bucket(ctx, z_index, material_id);
    append_draw_command(bucket, command);
    return command;
}

void
alia_draw_box(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_rgba color,
    float radius)
{
    auto* command
        = alia::downcast<alia_box_draw_command>(alia_draw_command_alloc(
            ctx,
            z_index,
            ALIA_BOX_MATERIAL_ID,
            ALIA_MIN_ALIGNED_SIZE(sizeof(alia_box_draw_command))));
    command->box = box;
    command->color = color;
    command->radius = radius;
}

} // extern "C"
