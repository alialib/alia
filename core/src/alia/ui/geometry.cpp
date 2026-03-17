#include <alia/abi/ui/geometry.h>
#include <alia/context.h>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>

extern "C" {

void
alia_geometry_push_clip_box(alia_context* ctx, alia_box box)
{
    alia_clip_state& clip = alia::stack_push<alia_clip_state>(ctx);
    alia_box* pointer = alia::arena_new<alia_box>(*ctx->scratch);
    *pointer = box;
    clip = ctx->geometry->clip;
    ctx->geometry->clip = {
        .box = box,
        .pointer = pointer,
        .id = 0,
    };
}

void
alia_geometry_pop_clip_box(alia_context* ctx)
{
    alia_clip_state& clip = alia::stack_pop<alia_clip_state>(ctx);
    ctx->geometry->clip = clip;
}
}
