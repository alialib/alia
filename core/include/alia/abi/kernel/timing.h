#ifndef ALIA_ABI_KERNEL_TIMING_H
#define ALIA_ABI_KERNEL_TIMING_H

#include <alia/abi/prelude.h>
#include <alia/context.hpp>

ALIA_EXTERN_C_BEGIN

static inline alia_nanosecond_count
alia_timing_tick_count(alia_context* ctx)
{
    return ctx->tick_count;
}

static inline void
alia_timing_request_animation_refresh(alia_context* ctx)
{
    // TODO: Implement this.
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_KERNEL_TIMING_H
