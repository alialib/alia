#ifndef ALIA_ABI_UI_DRAWING_TARGETS_H
#define ALIA_ABI_UI_DRAWING_TARGETS_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/commands.h>
#include <alia/abi/ui/geometry.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// Create a persistent offscreen draw target. Returns ALIA_DRAW_TARGET_PRIMARY
// on failure / unsupported renderer.
alia_draw_target_id
alia_draw_target_create(alia_ui_system* ui);

void
alia_draw_target_destroy(alia_ui_system* ui, alia_draw_target_id target);

// Ensure the target's backing store is at least `size` physical pixels.
void
alia_draw_target_ensure_size(
    alia_ui_system* ui, alia_draw_target_id target, alia_vec2i size);

// Use a substrate-owned draw target. This takes care of creating the target on
// first use and destroying it when the substrate slot is destroyed. Returns
// ALIA_DRAW_TARGET_PRIMARY if the creation fails.
alia_draw_target_id
alia_draw_target_use(alia_context* ctx);

// Begin a scope for drawing into `target`.
// Within this scope, draw content is recorded in within the target's
// coordinate space. `size` is in physical pixels.
void
alia_draw_target_begin(
    alia_context* ctx, alia_draw_target_id target, alia_vec2f size);

// End a draw target scope.
void
alia_draw_target_end(alia_context* ctx);

// Draw the contents of another draw target into the active draw target.
void
alia_draw_target(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_target_id source,
    alia_box dest,
    float opacity);

typedef struct alia_draw_target_command
{
    alia_draw_command base;
    alia_draw_target_id source;
    alia_box dest;
    float opacity;
} alia_draw_target_command;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_TARGETS_H */
