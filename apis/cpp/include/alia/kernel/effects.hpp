#pragma once

#include <alia/abi/base/arena.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/effect.h>
#include <alia/abi/prelude.h>

#include <new>
#include <type_traits>
#include <utility>

// This file defines C++ helpers for posting typed effects into the pass-local
// effect log.

namespace alia {

namespace detail {

// `call_effect` is an effect that invokes a callable at commit time.
template<class Fn>
struct call_effect
{
    alia_effect base;
    Fn fn;

    static void
    run(alia_effect* self)
    {
        auto* effect = reinterpret_cast<call_effect*>(self);
        effect->fn();
        effect->~call_effect();
    }
};

} // namespace detail

// Post `fn` to run after the controller returns. `fn` is moved into scratch
// storage and must remain valid to invoke at commit time.
template<class Fn>
void
post_call(alia_context* ctx, Fn fn, char const* label = nullptr)
{
    using effect_type = detail::call_effect<std::decay_t<Fn>>;
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->scratch);

    size_t const bytes = alia_min_aligned_size(sizeof(effect_type));
    size_t const align = alignof(effect_type) < ALIA_MIN_ALIGN
                           ? ALIA_MIN_ALIGN
                           : alignof(effect_type);
    void* mem = alia_arena_ptr(
        ctx->scratch, alia_arena_alloc_aligned(ctx->scratch, bytes, align));
    auto* effect = new (mem)
        effect_type{{&effect_type::run, label, nullptr}, std::move(fn)};
    alia_post_effect(ctx, &effect->base);
}

// Post an assignment of `value` into `*dst` at commit time.
template<class T>
void
post_assignment(
    alia_context* ctx, T* dst, T value, char const* label = nullptr)
{
    ALIA_ASSERT(dst);
    post_call(
        ctx,
        [dst, value = std::move(value)]() mutable { *dst = std::move(value); },
        label);
}

} // namespace alia
