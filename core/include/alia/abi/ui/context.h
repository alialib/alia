#ifndef ALIA_ABI_UI_CONTEXT_H
#define ALIA_ABI_UI_CONTEXT_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

static inline alia_ui_system*
alia_ctx_system(alia_context* ctx)
{
    return ctx->system;
}

static inline alia_style*
alia_ctx_style(alia_context* ctx)
{
    return ctx->style;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_CONTEXT_H */
