#include <alia/abi/kernel/effect.h>

#include <alia/abi/kernel/object.h>
#include <alia/abi/prelude.h>

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
alia_post_effect(alia_kernel_context* ctx, alia_effect* effect)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(effect);
    ALIA_ASSERT(effect->run);
    effect->next = nullptr;
    alia_effect_log* log = alia_ctx_effects(ctx);
    if (log->tail)
        log->tail->next = effect;
    else
        log->head = effect;
    log->tail = effect;
}

void
alia_post_write(
    alia_kernel_context* ctx,
    void* dst,
    void const* src,
    size_t size,
    char const* label)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(dst);
    ALIA_ASSERT(size == 0 || src != nullptr);

    alia_bump_allocator* scratch = alia_ctx_scratch(ctx);
    ALIA_ASSERT(scratch->arena);
    size_t const bytes = alia_min_aligned_size(sizeof(write_effect) + size);
    auto* write = reinterpret_cast<write_effect*>(
        alia_arena_ptr(scratch, alia_arena_alloc(scratch, bytes)));
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
    alia_post_effect(ctx, &write->base);
}

void
alia_run_effects(alia_kernel_context* ctx)
{
    ALIA_ASSERT(ctx);

    alia_effect_log* log = alia_ctx_effects(ctx);
    bool any = false;
    for (alia_effect* effect = log->head; effect != nullptr;
         effect = effect->next)
    {
        ALIA_ASSERT(effect->run);
        effect->run(effect);
        any = true;
    }
    log->head = nullptr;
    log->tail = nullptr;

    if (any && alia_ctx_object(ctx))
        alia_kernel_mark_dirty(alia_ctx_object(ctx));
}

} // extern "C"
