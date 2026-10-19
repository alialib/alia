#include <alia/abi/ui/geometry.h>
#include <alia/context.h>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>

using namespace alia::operators;

extern "C" {

void
alia_geometry_push_clip_box(alia_ui_context* ctx, alia_box box)
{
    alia_clip_state& saved_clip
        = alia::stack_push<alia_clip_state>(&ctx->kernel);
    alia_box* pointer = alia::arena_new<alia_box>(*ctx->kernel.scratch);
    *pointer = box;
    saved_clip = ctx->geometry->clip;
    ctx->geometry->clip = {
        .box = box,
        .pointer = pointer,
        .id = ctx->geometry->next_clip_id,
    };
    ++ctx->geometry->next_clip_id;
}

void
alia_geometry_pop_clip_box(alia_ui_context* ctx)
{
    alia_clip_state& saved_clip
        = alia::stack_pop<alia_clip_state>(&ctx->kernel);
    ctx->geometry->clip = saved_clip;
}

void
alia_geometry_push_translation(alia_ui_context* ctx, alia_vec2f offset)
{
    alia_vec2f& saved_offset = alia::stack_push<alia_vec2f>(&ctx->kernel);
    saved_offset = ctx->geometry->offset;
    ctx->geometry->offset += offset;
}

void
alia_geometry_pop_translation(alia_ui_context* ctx)
{
    alia_vec2f& saved_offset = alia::stack_pop<alia_vec2f>(&ctx->kernel);
    ctx->geometry->offset = saved_offset;
}

void
alia_geometry_push_scale(alia_ui_context* ctx, float factor)
{
    float& saved_scale = alia::stack_push<float>(&ctx->kernel);
    saved_scale = ctx->geometry->scale;
    ctx->geometry->scale *= factor;
}

void
alia_geometry_pop_scale(alia_ui_context* ctx)
{
    float& saved_scale = alia::stack_pop<float>(&ctx->kernel);
    ctx->geometry->scale = saved_scale;
}

void
alia_geometry_push_z_base(alia_ui_context* ctx, alia_z_index z_base)
{
    alia_z_index& saved_z_base = alia::stack_push<alia_z_index>(&ctx->kernel);
    saved_z_base = ctx->geometry->z_base;
    ctx->geometry->z_base = z_base;
}

void
alia_geometry_push_z_offset(alia_ui_context* ctx, alia_z_index delta)
{
    alia_geometry_push_z_base(ctx, ctx->geometry->z_base + delta);
}

void
alia_geometry_pop_z(alia_ui_context* ctx)
{
    alia_z_index& saved_z_base = alia::stack_pop<alia_z_index>(&ctx->kernel);
    ctx->geometry->z_base = saved_z_base;
}
}
