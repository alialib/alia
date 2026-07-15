#ifndef ALIA_ABI_UI_EFFECTS_H
#define ALIA_ABI_UI_EFFECTS_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>

ALIA_EXTERN_C_BEGIN

// EFFECTS
//
// Each effect is its own draw material (one program / PSO per material) so the
// bucket sorter groups instances by shader. Render backends allocate the
// material ID when the effect is registered.
//
// Draw commands carry an Alia surface-space region and an opaque param blob.
// How backends bind region / surface size / params to the GPU is
// renderer-specific (see each renderer's effect_register docs).

typedef struct alia_effect_style
{
    // Minimum layout size in logical pixels.
    alia_vec2f min_size;
} alia_effect_style;

typedef struct alia_effect_draw_command
{
    alia_draw_command base;
    // Allocated layout region in surface space (top-left origin).
    alia_box region;
    uint16_t params_size;
    // Points into the draw-pass arena; valid only during draw_bucket.
    void const* params;
} alia_effect_draw_command;

alia_effect_style const*
alia_default_effect_style(void);

// Record an effect draw for a known surface-space box.
// `material_id` is the ID returned when the effect was registered.
void
alia_draw_effect(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    alia_box region,
    void const* params,
    size_t params_size);

// Layout leaf that records an effect over its allocated box.
alia_element_id
alia_do_effect(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    void const* params,
    size_t params_size,
    alia_layout_flags_t layout_flags,
    alia_effect_style const* style);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_EFFECTS_H */
