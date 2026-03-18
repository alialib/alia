#include <alia/abi/ui/geometry.h>
#include <alia/context.h>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>

using namespace alia::operators;

extern "C" {

void
alia_geometry_push_clip_box(alia_context* ctx, alia_box box)
{
    alia_clip_state& saved_clip = alia::stack_push<alia_clip_state>(ctx);
    alia_box* pointer = alia::arena_new<alia_box>(*ctx->scratch);
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
alia_geometry_pop_clip_box(alia_context* ctx)
{
    alia_clip_state& saved_clip = alia::stack_pop<alia_clip_state>(ctx);
    ctx->geometry->clip = saved_clip;
}

void
alia_geometry_push_translation(alia_context* ctx, alia_vec2f offset)
{
    alia_vec2f& saved_offset = alia::stack_push<alia_vec2f>(ctx);
    saved_offset = ctx->geometry->offset;
    ctx->geometry->offset += offset;
}

void
alia_geometry_pop_translation(alia_context* ctx)
{
    alia_vec2f& saved_offset = alia::stack_pop<alia_vec2f>(ctx);
    ctx->geometry->offset = saved_offset;
}
}
