#ifndef ALIA_ABI_KERNEL_CONTEXT_H
#define ALIA_ABI_KERNEL_CONTEXT_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

// This file grants access to the kernel-level capabilities of the context.

ALIA_EXTERN_C_BEGIN

static inline alia_event_traversal*
alia_ctx_events(alia_context* ctx)
{
    return ctx->events;
}

static inline alia_kernel*
alia_ctx_kernel(alia_context* ctx)
{
    return ctx->kernel;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_CONTEXT_H */
