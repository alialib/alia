#ifndef ALIA_ABI_UI_STYLING_H
#define ALIA_ABI_UI_STYLING_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/palette.h>
#include <stddef.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// theme-time style seeds - These are parameters into the style generation
// functions.
typedef struct alia_style_seeds
{
    // layout rhythm (sibling gap, etc.), logical px
    float spacing;
    // control chrome size multiplier (1 = reference defaults)
    float scale;
    // corner language from 0 (square) to 1 (as round as each control allows)
    float roundness;
} alia_style_seeds;

// Return the default style seeds.
alia_style_seeds
alia_style_seeds_default(void);

// layout style (catalog slot ALIA_STYLE_LAYOUT)
typedef struct alia_layout_style
{
    float spacing;
} alia_layout_style;

// Fill `out` with the layout style for `seeds`.
// Seeds can be `NULL` to use the default seeds.
void
alia_layout_style_generate(
    alia_layout_style* out, alia_style_seeds const* seeds);

// focus ring chrome (catalog slot ALIA_STYLE_FOCUS)
typedef struct alia_focus_style
{
    alia_palette_color color;
    // outset beyond the control bounds, logical px
    float outset;
    float thickness;
    float corner_radius;
    // show-on-pointer policy - When false, draw focus only while
    // input->keyboard_interaction is set.
    bool show_on_pointer_focus;
} alia_focus_style;

// Fill `out` with the focus style for `seeds`.
// Seeds can be `NULL` to use the default seeds.
void
alia_focus_style_generate(
    alia_focus_style* out, alia_style_seeds const* seeds);

// built-in style slot indices - Third-party slots start at
// ALIA_STYLE_BUILTIN_COUNT via alia_style_slot_add.
enum alia_style_slot
{
    ALIA_STYLE_LAYOUT = 0,
    ALIA_STYLE_TEXT,
    ALIA_STYLE_FOCUS,
    ALIA_STYLE_SWITCH,
    ALIA_STYLE_SLIDER,
    ALIA_STYLE_RADIO,
    ALIA_STYLE_CHECKBOX,
    ALIA_STYLE_NODE_EXPANDER,
    ALIA_STYLE_SCROLLBAR,
    ALIA_STYLE_BUILTIN_COUNT
};

typedef uint32_t alia_style_slot_id;

// maximum number of style slots (built-in + third-party)
#define ALIA_STYLE_SLOT_MAX 64

// style generator function - This describes the callback used to fill out
// a slot's default style from the style seeds. (If other parameters are
// desired, they can be communicated via `user_data`.) `out` is a pointer to
// the slot's style blob, allocated according to the slot's size and alignment.
typedef void (*alia_style_generator_fn)(
    void* user_data, void* out, alia_style_seeds const* seeds);

// Add a third-party slot with its generator and return its ID. Call
// `alia_style_generate_defaults` before the slot is used.
alia_style_slot_id
alia_style_slot_add(
    alia_ui_system* ui,
    size_t size,
    size_t align,
    alia_style_generator_fn generator,
    void* user_data);

// Replace the generator for an existing slot (built-in or third-party).
// Note that a call to `alia_style_generate_defaults` is required for this to
// be reflected in the actual default style.
void
alia_style_slot_set_generator(
    alia_ui_system* ui,
    alia_style_slot_id slot,
    alia_style_generator_fn generator,
    void* user_data);

// Run all slot generators to fill out the system defaults blob.
// If seeds is `NULL`, the system's current seeds are used (or the default
// seeds if no seeds have been set yet).
void
alia_style_generate_defaults(
    alia_ui_system* ui, alia_style_seeds const* seeds);

// Return the system-owned default for a slot. This can be used to perform
// manual modifications immediately after generation.
void*
alia_style_default(alia_ui_system* ui, alia_style_slot_id slot);

// Return the context-owned mutable active for a slot. This is a per-pass copy
// of the default. It can be used to perform scoped modifications during UI
// traversal.
void*
alia_style_active(alia_context* ctx, alia_style_slot_id slot);

static inline alia_layout_style*
alia_layout_style_default(alia_ui_system* ui)
{
    return (alia_layout_style*) alia_style_default(ui, ALIA_STYLE_LAYOUT);
}

static inline alia_layout_style*
alia_layout_style_active(alia_context* ctx)
{
    // `Layout` is always the first catalog slot, so its offset in the actives
    // blob is 0. A consequence of this is that Layout-only test fixtures can
    // point `active_styles` at a lone `alia_layout_style` without a UI system.
    ALIA_ASSERT(ctx && ctx->active_styles);
    return (alia_layout_style*) ctx->active_styles;
}

static inline alia_focus_style*
alia_focus_style_default(alia_ui_system* ui)
{
    return (alia_focus_style*) alia_style_default(ui, ALIA_STYLE_FOCUS);
}

static inline alia_focus_style*
alia_focus_style_active(alia_context* ctx)
{
    return (alia_focus_style*) alia_style_active(ctx, ALIA_STYLE_FOCUS);
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_STYLING_H */
