#include <alia/drawing.hpp>

#include <alia/arena.hpp>
#include <alia/context.hpp>
#include <alia/drawing.hpp>

#include <alia/internals/drawing.hpp>

namespace alia {

alia_draw_material_id box_material_id = 0;

void
draw_box(
    alia_draw_context* ctx,
    alia_z_index z_index,
    alia_box box,
    alia_rgba color)
{
    box_draw_command* command = arena_alloc<box_draw_command>(*ctx->arena);
    command->box = box;
    command->color = color;
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
    return &ctx->buckets->buckets[alia::make_bucket_key(z_index, material_id)];
}

} // extern "C"
