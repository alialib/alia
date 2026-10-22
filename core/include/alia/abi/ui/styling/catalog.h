#ifndef ALIA_ABI_UI_STYLING_CATALOG_H
#define ALIA_ABI_UI_STYLING_CATALOG_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/context.h>

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

// Set the ambient style fields on `style` from `seeds`.
// `seeds` may be NULL to use the default seeds.
void
alia_style_apply_seeds(
    alia_style_context* style, alia_style_seeds const* seeds);

// built-in style slot indices - Third-party slots start at
// ALIA_STYLE_BUILTIN_COUNT via alia_style_slot_add.
enum alia_style_slot
{
    ALIA_STYLE_TOGGLE_SWITCH = 0,
    ALIA_STYLE_SLIDER,
    ALIA_STYLE_RADIO_BUTTON,
    ALIA_STYLE_CHECKBOX,
    ALIA_STYLE_NODE_EXPANDER,
    ALIA_STYLE_SCROLLBAR,
    ALIA_STYLE_BUTTON,
    ALIA_STYLE_SEPARATOR,
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
// If `seeds` is `NULL`, the system's current seeds are used (or the default
// seeds if no seeds have been set yet).
// This calls `alia_ui_metrics_changed` internally.
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
alia_style_active(alia_ui_context* ctx, alia_style_slot_id slot);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_STYLING_CATALOG_H */
