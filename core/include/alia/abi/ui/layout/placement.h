#ifndef ALIA_ABI_UI_LAYOUT_PLACEMENT_H
#define ALIA_ABI_UI_LAYOUT_PLACEMENT_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_context alia_ui_context;
typedef struct alia_layout_system alia_layout_system;
typedef struct alia_layout_node alia_layout_node;

// generation counter for placement data - This is bumped by
// `alia_layout_placement_clear` when the placement arena starts a new epoch.
typedef uint64_t alia_layout_placement_generation;

// a contiguous slice of the shared placement arena written by one resolution
typedef struct alia_layout_placement_region
{
    alia_offset offset;
} alia_layout_placement_region;

// Clear out the placement arena and begin a new generation of bookkeeping.
// (This does not free the placement arena.)
void
alia_layout_placement_clear(alia_layout_system* system);

// Get the current placement generation.
alia_layout_placement_generation
alia_layout_placement_current_generation(alia_layout_system const* system);

// Resolve `root` into the next placement region, appending to the shared
// placement arena. Assigns `root` into a box at (0, 0) with size
// `available_space`. Returns the start of the placement region for later
// consumption.
alia_layout_placement_region
alia_layout_placement_resolve(
    alia_layout_system* system,
    alia_layout_node* root,
    alia_vec2f available_space);

// Redirect the context's placement consume cursor to `region`.
// (This is nestable and should be paired with `alia_layout_placement_pop`.)
void
alia_layout_placement_push(
    alia_ui_context* ctx, alia_layout_placement_region region);

void
alia_layout_placement_pop(alia_ui_context* ctx);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_PLACEMENT_H
