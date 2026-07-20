#ifndef ALIA_ABI_UI_SYSTEM_RENDERER_H
#define ALIA_ABI_UI_SYSTEM_RENDERER_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/effects.h>
#include <alia/abi/ui/drawing/system.h>
#include <alia/abi/ui/drawing/targets.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/msdf.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// renderer-provided GPU hooks
typedef struct alia_renderer_ops
{
    void (*upload_msdf_atlas)(void* user, alia_msdf_atlas_image const* image);

    // Register a portable effect. Returns 0 on success.
    int (*register_effect)(
        void* user,
        alia_effect_desc const* desc,
        alia_draw_material_id* out_material_id);

    // Create/destroy/resize an offscreen draw target.
    alia_draw_target_id (*draw_target_create)(void* user);
    void (*draw_target_destroy)(void* user, alia_draw_target_id target);
    void (*draw_target_ensure_size)(
        void* user, alia_draw_target_id target, alia_vec2i size);

    // Begin/end a draw pass.
    void (*draw_pass_begin)(void* user);
    void (*draw_pass_end)(void* user);

    // Bind a draw target to the active draw target.
    void (*draw_target_bind)(void* user, alia_draw_target_id target);

    // Clear a draw target to a solid color.
    void (*draw_target_clear)(
        void* user, alia_draw_target_id target, float const rgba[4]);

    // user data for the renderer
    void* user;
} alia_renderer_ops;

void
alia_ui_system_set_renderer_ops(
    alia_ui_system* ui, alia_renderer_ops const* ops);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_SYSTEM_RENDERER_H */
