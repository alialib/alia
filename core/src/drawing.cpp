#include <alia/drawing.hpp>

#include <alia/arena.hpp>
#include <alia/context.hpp>
#include <alia/drawing.hpp>

#include <alia/internals/drawing.hpp>

#include <iostream>

namespace alia {

alia_draw_material_id box_material_id = 0;

void
draw_box(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_rgba color,
    float radius)
{
    box_draw_command* command = arena_alloc<box_draw_command>(*ctx->arena);
    command->box = box;
    command->color = color;
    command->radius = radius;
    alia_draw_bucket_append(
        alia_get_draw_bucket(ctx, z_index, box_material_id),
        upcast<alia_draw_command>(command));
}

} // namespace alia

extern "C" {

alia_draw_material_id
alia_register_material(
    alia_draw_system* system, alia_material_vtable vtable, void* user)
{
    auto const material_id = alia_draw_material_id(system->materials.size());
    system->materials.push_back(
        alia_draw_material{.vtable = vtable, .user = user});
    return material_id;
}

alia_draw_bucket*
alia_get_draw_bucket(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id)
{
    auto const key = alia::make_bucket_key(z_index, material_id);
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

} // extern "C"
