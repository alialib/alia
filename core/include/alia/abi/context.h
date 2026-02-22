#ifndef ALIA_ABI_CONTEXT_H
#define ALIA_ABI_CONTEXT_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_kernel alia_kernel;
typedef struct alia_event_traversal alia_event_traversal;
typedef struct alia_stack alia_stack;
typedef struct alia_ui_system alia_ui_system;
typedef struct alia_style alia_style;
typedef struct alia_geometry_context alia_geometry_context;
typedef struct alia_input_state alia_input_state;
typedef struct alia_layout_context alia_layout_context;
typedef struct alia_substrate_traversal alia_substrate_traversal;

typedef struct alia_context
{
    // kernel-level capabilities
    alia_kernel* kernel;
    alia_substrate_traversal* substrate;
    alia_event_traversal* events;
    alia_stack* stack;

    // UI-level capabilities
    alia_ui_system* system;
    alia_style* style;
    alia_geometry_context* geometry;
    alia_input_state* input;
    alia_layout_context* layout;
} alia_context;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_CONTEXT_H */
