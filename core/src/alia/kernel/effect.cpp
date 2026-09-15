#include <alia/abi/kernel/effect.h>

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/work.h>
#include <alia/impl/events.hpp>

#include <cstring>

namespace {

struct write_effect
{
    alia_effect base;
    void* dst;
    size_t size;
};

void
write_effect_run(alia_effect* self)
{
    auto* write = reinterpret_cast<write_effect*>(self);
    if (write->size == 0)
        return;
    std::memcpy(
        write->dst,
        reinterpret_cast<unsigned char const*>(write) + sizeof(write_effect),
        write->size);
}

} // namespace

extern "C" {

void
alia_defer_effect(alia_context* ctx, alia_effect* effect)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->effects);
    ALIA_ASSERT(effect);
    ALIA_ASSERT(effect->run);
    effect->next = nullptr;
    if (ctx->effects->tail)
        ctx->effects->tail->next = effect;
    else
        ctx->effects->head = effect;
    ctx->effects->tail = effect;
}

void
alia_defer_write(
    alia_context* ctx,
    void* dst,
    void const* src,
    size_t size,
    char const* label)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->scratch);
    ALIA_ASSERT(dst);
    ALIA_ASSERT(size == 0 || src != nullptr);

    size_t const bytes = alia_min_aligned_size(sizeof(write_effect) + size);
    auto* write = reinterpret_cast<write_effect*>(
        alia_arena_ptr(ctx->scratch, alia_arena_alloc(ctx->scratch, bytes)));
    write->base.run = write_effect_run;
    write->base.label = label;
    write->base.next = nullptr;
    write->dst = dst;
    write->size = size;
    if (size != 0)
    {
        std::memcpy(
            reinterpret_cast<unsigned char*>(write) + sizeof(write_effect),
            src,
            size);
    }
    alia_defer_effect(ctx, &write->base);
}

void
alia_run_effects(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->effects);
    if (ctx->events && ctx->events->aborted)
        return;

    bool any = false;
    for (alia_effect* effect = ctx->effects->head; effect != nullptr;
         effect = effect->next)
    {
        ALIA_ASSERT(effect->run);
        effect->run(effect);
        any = true;
    }
    ctx->effects->head = nullptr;
    ctx->effects->tail = nullptr;

    if (any && ctx->system)
        alia_ui_mark_dirty(ctx->system);
}

} // extern "C"
