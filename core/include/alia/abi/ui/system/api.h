#ifndef ALIA_ABI_UI_SYSTEM_API_H
#define ALIA_ABI_UI_SYSTEM_API_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

typedef void (*alia_ui_controller_fn)(void* user_data, alia_context* ctx);

typedef struct alia_ui_controller
{
    alia_ui_controller_fn fn;
    void* user_data;
} alia_ui_controller;

// size and alignment for placement-new / malloc storage of `alia_ui_system`
alia_struct_spec
alia_ui_system_object_spec(void);

// `controller.fn` must be non-null. `controller.user_data` may be null.
// `object_storage` must point to at least `alia_ui_system_object_spec().size`
// bytes aligned to `alia_ui_system_object_spec().align`.
alia_ui_system*
alia_ui_system_init(
    void* object_storage,
    alia_ui_controller controller,
    alia_vec2i surface_size);

// Detects changes in UI contents, updates layout, and processes input routing.
void
alia_ui_system_update(alia_ui_system* ui);

void
alia_ui_surface_set_size(alia_ui_system* ui, alia_vec2i new_size);

alia_vec2i
alia_ui_surface_get_size(alia_ui_system* ui);

void
alia_ui_surface_set_dpi(alia_ui_system* ui, float dpi);

float
alia_ui_surface_get_dpi(alia_ui_system* ui);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_SYSTEM_API_H */
