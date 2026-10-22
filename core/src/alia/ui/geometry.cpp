#include <alia/abi/ui/geometry/api.h>
#include <alia/impl/context.hpp>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>

using namespace alia::operators;

extern "C" {

void
alia_geometry_push_clip_box(alia_ui_context* ctx, alia_box box)
{
    alia_clip_state& saved_clip
        = alia::stack_push<alia_clip_state>(alia_ctx_kernel(ctx));
    alia_box* pointer
        = alia::arena_new<alia_box>(*alia_ctx_scratch(alia_ctx_kernel(ctx)));
    *pointer = box;
    saved_clip = alia_ctx_geometry(ctx)->clip;
    alia_ctx_geometry(ctx)->clip = {
        .box = box,
        .pointer = pointer,
        .id = alia_ctx_geometry(ctx)->next_clip_id,
    };
    ++alia_ctx_geometry(ctx)->next_clip_id;
}

void
alia_geometry_pop_clip_box(alia_ui_context* ctx)
{
    alia_clip_state& saved_clip
        = alia::stack_pop<alia_clip_state>(alia_ctx_kernel(ctx));
    alia_ctx_geometry(ctx)->clip = saved_clip;
}

void
alia_geometry_push_translation(alia_ui_context* ctx, alia_vec2f offset)
{
    alia_vec2f& saved_offset
        = alia::stack_push<alia_vec2f>(alia_ctx_kernel(ctx));
    saved_offset = alia_ctx_geometry(ctx)->offset;
    alia_ctx_geometry(ctx)->offset += offset;
}

void
alia_geometry_pop_translation(alia_ui_context* ctx)
{
    alia_vec2f& saved_offset
        = alia::stack_pop<alia_vec2f>(alia_ctx_kernel(ctx));
    alia_ctx_geometry(ctx)->offset = saved_offset;
}

void
alia_geometry_push_scale(alia_ui_context* ctx, float factor)
{
    float& saved_scale = alia::stack_push<float>(alia_ctx_kernel(ctx));
    saved_scale = alia_ctx_geometry(ctx)->scale;
    alia_ctx_geometry(ctx)->scale *= factor;
}

void
alia_geometry_pop_scale(alia_ui_context* ctx)
{
    float& saved_scale = alia::stack_pop<float>(alia_ctx_kernel(ctx));
    alia_ctx_geometry(ctx)->scale = saved_scale;
}

void
alia_geometry_push_z_base(alia_ui_context* ctx, alia_z_index z_base)
{
    alia_z_index& saved_z_base
        = alia::stack_push<alia_z_index>(alia_ctx_kernel(ctx));
    saved_z_base = alia_ctx_geometry(ctx)->z_base;
    alia_ctx_geometry(ctx)->z_base = z_base;
}

void
alia_geometry_push_z_offset(alia_ui_context* ctx, alia_z_index delta)
{
    alia_geometry_push_z_base(ctx, alia_ctx_geometry(ctx)->z_base + delta);
}

void
alia_geometry_pop_z(alia_ui_context* ctx)
{
    alia_z_index& saved_z_base
        = alia::stack_pop<alia_z_index>(alia_ctx_kernel(ctx));
    alia_ctx_geometry(ctx)->z_base = saved_z_base;
}
}
