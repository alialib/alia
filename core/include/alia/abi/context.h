#ifndef ALIA_ABI_CONTEXT_H
#define ALIA_ABI_CONTEXT_H

#include <alia/abi/kernel/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/palette.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;
typedef struct alia_resolved_font alia_resolved_font;
typedef struct alia_geometry_context alia_geometry_context;
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
    // mutable active styles for this pass - These are initialized to copies of
    // the system defaults when the context is created.
    void* active_styles;
    // NULL until a font is pushed
    alia_resolved_font const* active_font;
    // active text color
    alia_srgba8 text_color;
    alia_geometry_context* geometry;
    alia_input_state* input;
    alia_layout_context* layout;
    alia_draw_context* draw;
    alia_palette* palette;
} alia_ui_context;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_CONTEXT_H */
