#ifndef ALIA_ABI_UI_DRAWING_SYSTEM_H
#define ALIA_ABI_UI_DRAWING_SYSTEM_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/commands.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// built-in material IDs
enum
{
    ALIA_PRIMITIVE_MATERIAL_ID = 0,
    // Composite a draw target texture into the current target.
    ALIA_DRAW_TARGET_MATERIAL_ID = 1,
    // TODO: monospaced/variable-width debug text material IDs
    ALIA_BUILTIN_MATERIAL_COUNT = 2,
};

// Allocate IDs for custom materials.
alia_draw_material_id
alia_material_alloc_ids(alia_ui_system* system, uint32_t count);

typedef void (*alia_material_draw_fn)(
    void* user, alia_draw_bucket const* bucket);

typedef struct alia_material_vtable
{
    alia_material_draw_fn draw_bucket;
} alia_material_vtable;

// Register the vtable for a material.
// The ID can either be a built-in ID or a custom ID allocated with
// `alia_material_alloc_ids()`.
void
alia_material_register(
    alia_ui_system* system,
    alia_draw_material_id id,
    alia_material_vtable vtable,
    void* user);

// Execute a draw pass.
// This invokes the UI controller to record draw commands, then invokes the
// associated material vtables to execute the draw commands in the appropriate
// order.
void
alia_ui_execute_draw_pass(alia_ui_system* system);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_SYSTEM_H */
