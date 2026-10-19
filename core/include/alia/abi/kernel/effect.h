#ifndef ALIA_ABI_KERNEL_EFFECT_H
#define ALIA_ABI_KERNEL_EFFECT_H

#include <alia/abi/kernel/context.h>
#include <alia/abi/prelude.h>

#include <stddef.h>

ALIA_EXTERN_C_BEGIN

// Append `effect` to this pass's FIFO log. Effects are generally allocated
// from the pass's scratch arena. Any referenced data should either be copied
// there or should remain valid until the end of the pass.
void
alia_post_effect(alia_kernel_context* ctx, alia_effect* effect);

// Post a memcpy of `size` bytes from `src` to `dst`. `label` is optional.
void
alia_post_write(
    alia_kernel_context* ctx,
    void* dst,
    void const* src,
    size_t size,
    char const* label);

// Run posted effects in FIFO order.
void
alia_run_effects(alia_kernel_context* ctx);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_EFFECT_H */
