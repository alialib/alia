#ifndef ALIA_ABI_KERNEL_EFFECT_H
#define ALIA_ABI_KERNEL_EFFECT_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

#include <stddef.h>

ALIA_EXTERN_C_BEGIN

// a deferred pass-local operation - Alia requires that component traversals
// are mutation-free, so effects are recorded during a pass and run after the
// controller returns.
typedef struct alia_effect alia_effect;
struct alia_effect
{
    // function that applies this effect
    void (*run)(alia_effect* self);
    // debug label
    char const* label;
    // next posted effect
    alia_effect* next;
};

// FIFO of deferred pass-local operations
typedef struct alia_effect_log
{
    alia_effect* head;
    alia_effect* tail;
} alia_effect_log;

// Append `effect` to this pass's FIFO log. Effects are generally allocated
// from the pass's scratch arena. Any referenced data should either be copied
// there or should remain valid until the end of the pass.
void
alia_defer_effect(alia_context* ctx, alia_effect* effect);

// Post a memcpy of `size` bytes from `src` to `dst`. `label` is optional.
void
alia_defer_write(
    alia_context* ctx,
    void* dst,
    void const* src,
    size_t size,
    char const* label);

// Run posted effects in FIFO order.
void
alia_run_effects(alia_context* ctx);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_EFFECT_H */
