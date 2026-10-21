#ifndef ALIA_ABI_UI_SYSTEM_MEMORY_H
#define ALIA_ABI_UI_SYSTEM_MEMORY_H

#include <alia/abi/kernel/memory.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// UI pool name - The value is not a pool ID.
typedef enum alia_ui_pool_name
{
    ALIA_UI_POOL_LAYOUT_NODES = 0,
    ALIA_UI_POOL_LAYOUT_SCRATCH,
    ALIA_UI_POOL_LAYOUT_PLACEMENT,
    ALIA_UI_POOL_DRAW_COMMANDS,
} alia_ui_pool_name;

// Return the pool ID registered for `name`.
alia_memory_pool_id
alia_ui_pool(alia_ui_system* ui, alia_ui_pool_name name);

alia_memory_system*
alia_ui_memory(alia_ui_system* ui);

ALIA_EXTERN_C_END

#endif
