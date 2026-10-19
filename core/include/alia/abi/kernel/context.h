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
typedef struct alia_effect_log alia_effect_log;

// `alia_kernel_context` is the pass-local context for kernel operations.
typedef struct alia_kernel_context
{
    // persistent kernel object
    alia_kernel* object;
    // pass-local substrate traversal
    alia_substrate_traversal* substrate;
    alia_event_traversal* events;
    alia_stack* stack;
    alia_bump_allocator* scratch;
    // tick count for this pass
    alia_nanosecond_count tick_count;
    alia_effect_log* effects;
} alia_kernel_context;

static inline alia_event_traversal*
alia_ctx_events(alia_kernel_context* ctx)
{
    return ctx->events;
}

static inline alia_kernel*
alia_ctx_kernel(alia_kernel_context* ctx)
{
    return ctx->object;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_CONTEXT_H */
