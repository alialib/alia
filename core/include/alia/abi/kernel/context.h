#ifndef ALIA_ABI_KERNEL_CONTEXT_H
#define ALIA_ABI_KERNEL_CONTEXT_H

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>

// This file defines the pass-local kernel context.

ALIA_EXTERN_C_BEGIN

typedef struct alia_kernel alia_kernel;
typedef struct alia_substrate_traversal alia_substrate_traversal;
typedef struct alia_event_traversal alia_event_traversal;
typedef struct alia_stack alia_stack;

// a pass-local effect posted during traversal - Alia requires that passes
// through component code are mutation-free, so instead of in-place mutations,
// components post effect objects that are run after the pass completes.
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

// FIFO of posted pass-local operations
typedef struct alia_effect_log
{
    alia_effect* head;
    alia_effect* tail;
} alia_effect_log;

// `alia_kernel_context` is the pass-local context for kernel operations.
typedef struct alia_kernel_context
{
    // persistent kernel object
    alia_kernel* object;
    // pass-local substrate traversal
    alia_substrate_traversal* substrate;
    // pass-local event traversal
    alia_event_traversal* events;
    // pass-local stack
    alia_stack* stack;
    // pass-local scratch allocator
    alia_bump_allocator scratch;
    // tick count for this pass
    alia_nanosecond_count tick_count;
    // pass-local effect log
    alia_effect_log effects;
} alia_kernel_context;

static inline alia_kernel*
alia_ctx_object(alia_kernel_context* ctx)
{
    return ctx->object;
}

static inline alia_substrate_traversal*
alia_ctx_substrate(alia_kernel_context* ctx)
{
    return ctx->substrate;
}

static inline alia_event_traversal*
alia_ctx_events(alia_kernel_context* ctx)
{
    return ctx->events;
}

static inline alia_stack*
alia_ctx_stack(alia_kernel_context* ctx)
{
    return ctx->stack;
}

static inline alia_bump_allocator*
alia_ctx_scratch(alia_kernel_context* ctx)
{
    return &ctx->scratch;
}

static inline alia_effect_log*
alia_ctx_effects(alia_kernel_context* ctx)
{
    return &ctx->effects;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_CONTEXT_H */
