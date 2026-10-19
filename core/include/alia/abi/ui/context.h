#ifndef ALIA_ABI_UI_CONTEXT_H
#define ALIA_ABI_UI_CONTEXT_H

#include <alia/abi/kernel/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/geometry/types.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling/context.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;
typedef struct alia_input_state alia_input_state;
typedef struct alia_layout_context alia_layout_context;
typedef struct alia_draw_context alia_draw_context;

// `alia_ui_context` is the pass-local context for a UI traversal.
// Its first member is the kernel context, so the two share the same address.
typedef struct alia_ui_context
{
    // kernel context for this pass
    alia_kernel_context kernel;

    // UI-level capabilities
    alia_ui_system* system;
    // pass-local style state
    alia_style_context style;
    // pass-local geometry
    alia_geometry_context geometry;
    alia_input_state* input;
    alia_layout_context* layout;
    // NULL when this pass is not drawing
    alia_draw_context* draw;
    alia_palette* palette;
} alia_ui_context;

static inline alia_kernel_context*
alia_ctx_kernel(alia_ui_context* ctx)
{
    return &ctx->kernel;
}

static inline alia_ui_system*
alia_ctx_system(alia_ui_context* ctx)
{
    return ctx->system;
}

static inline alia_style_context*
alia_ctx_style(alia_ui_context* ctx)
{
    return &ctx->style;
}

static inline alia_geometry_context*
alia_ctx_geometry(alia_ui_context* ctx)
{
    return &ctx->geometry;
}

static inline alia_input_state*
alia_ctx_input(alia_ui_context* ctx)
{
    return ctx->input;
}

static inline alia_layout_context*
alia_ctx_layout(alia_ui_context* ctx)
{
    return ctx->layout;
}

static inline alia_draw_context*
alia_ctx_draw(alia_ui_context* ctx)
{
    return ctx->draw;
}

static inline alia_palette*
alia_ctx_palette(alia_ui_context* ctx)
{
    return ctx->palette;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_CONTEXT_H */
