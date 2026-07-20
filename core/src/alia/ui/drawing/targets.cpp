#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/commands.h>
#include <alia/abi/ui/drawing/system.h>
#include <alia/abi/ui/drawing/targets.h>
#include <alia/abi/ui/system/renderer.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/context.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/ui/system/object.h>

namespace {

struct draw_target_scope
{
    alia_draw_target_id target_id;
    alia_vec2f offset;
    alia_clip_state clip;
};

struct draw_target_slot
{
    alia_ui_system* ui = nullptr;
    alia_draw_target_id id = ALIA_DRAW_TARGET_PRIMARY;
};

void
draw_target_slot_cleanup(
    alia_substrate_system*, void* ptr, alia_substrate_cleanup_mode mode)
{
    auto* slot = static_cast<draw_target_slot*>(ptr);
    if (mode == ALIA_SUBSTRATE_DESTROY || mode == ALIA_SUBSTRATE_CLEAR_CACHE)
    {
        if (slot->ui && slot->id != ALIA_DRAW_TARGET_PRIMARY)
            alia_draw_target_destroy(slot->ui, slot->id);
        slot->id = ALIA_DRAW_TARGET_PRIMARY;
        slot->ui = nullptr;
    }
}

} // namespace

extern "C" {

alia_draw_target_id
alia_draw_target_create(alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    if (!ui->renderer.draw_target_create)
        return ALIA_DRAW_TARGET_PRIMARY;
    return ui->renderer.draw_target_create(ui->renderer.user);
}

void
alia_draw_target_destroy(alia_ui_system* ui, alia_draw_target_id target)
{
    ALIA_ASSERT(ui);
    if (target == ALIA_DRAW_TARGET_PRIMARY)
        return;
    if (ui->renderer.draw_target_destroy)
        ui->renderer.draw_target_destroy(ui->renderer.user, target);
}

void
alia_draw_target_ensure_size(
    alia_ui_system* ui, alia_draw_target_id target, alia_vec2i size)
{
    ALIA_ASSERT(ui);
    if (target == ALIA_DRAW_TARGET_PRIMARY)
        return;
    if (ui->renderer.draw_target_ensure_size)
        ui->renderer.draw_target_ensure_size(ui->renderer.user, target, size);
}

alia_draw_target_id
alia_draw_target_use(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    alia_substrate_usage_result const result = alia_substrate_use_object(
        ctx,
        sizeof(draw_target_slot),
        alignof(draw_target_slot),
        draw_target_slot_cleanup);
    auto* slot = reinterpret_cast<draw_target_slot*>(result.ptr);
    bool const fresh = result.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
    if (fresh)
    {
        *slot = draw_target_slot{};
        slot->ui = ctx->system;
        slot->id = alia_draw_target_create(ctx->system);
    }
    return slot->id;
}

void
alia_draw_target_begin(
    alia_context* ctx, alia_draw_target_id target, alia_vec2f size)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->geometry);
    ALIA_ASSERT(ctx->draw);
    ALIA_ASSERT(target != ALIA_DRAW_TARGET_PRIMARY);
    ALIA_ASSERT(size.x > 0.f && size.y > 0.f);

    alia_vec2i const pixel_size
        = {int(size.x + 0.5f), int(size.y + 0.5f)};
    alia_draw_target_ensure_size(ctx->system, target, pixel_size);

    auto& saved = alia::stack_push<draw_target_scope>(ctx);
    saved = {
        .target_id = ctx->draw->target_id,
        .offset = ctx->geometry->offset,
        .clip = ctx->geometry->clip,
    };

    alia_box* clip_pointer = alia::arena_new<alia_box>(*ctx->scratch);
    *clip_pointer = alia_box{{0.f, 0.f}, size};

    ctx->draw->target_id = target;
    ctx->geometry->offset = {0.f, 0.f};
    ctx->geometry->clip = {
        .box = *clip_pointer,
        .pointer = clip_pointer,
        .id = ctx->geometry->next_clip_id,
    };
    ++ctx->geometry->next_clip_id;
}

void
alia_draw_target_end(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->geometry);
    ALIA_ASSERT(ctx->draw);
    auto& saved = alia::stack_pop<draw_target_scope>(ctx);
    ctx->draw->target_id = saved.target_id;
    ctx->geometry->offset = saved.offset;
    ctx->geometry->clip = saved.clip;
}

void
alia_draw_target(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_target_id source,
    alia_box dest,
    float opacity)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->draw);
    ALIA_ASSERT(source != ALIA_DRAW_TARGET_PRIMARY);
    ALIA_ASSERT(source != ctx->draw->target_id);

    auto* command = (alia_draw_target_command*) alia_draw_command_alloc(
        ctx,
        z_index,
        ALIA_DRAW_TARGET_MATERIAL_ID,
        ALIA_MIN_ALIGNED_SIZE(sizeof(alia_draw_target_command)));
    command->source = source;
    command->dest = alia_box_translate(dest, ctx->geometry->offset);
    command->opacity = opacity;
}

} // extern "C"
