#ifndef ALIA_ABI_CONTEXT_H
#define ALIA_ABI_CONTEXT_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_kernel alia_kernel;
typedef struct alia_event_traversal alia_event_traversal;
typedef struct alia_ui_system alia_ui_system;
typedef struct alia_style alia_style;

typedef struct alia_context
{
    // kernel-level capabilities
    alia_kernel* kernel;
    alia_event_traversal* events;

    // UI-level capabilities
    alia_ui_system* system;
    alia_style* style;
} alia_context;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_CONTEXT_H */
