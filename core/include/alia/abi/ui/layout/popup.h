#ifndef ALIA_ABI_UI_LAYOUT_POPUP_H
#define ALIA_ABI_UI_LAYOUT_POPUP_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

// Place a popup on one side of `anchor`, inside `bounds`.
// Return the placement box.
//
// `anchor`, `preferred_size`, `min_size`, and `bounds` are all in surface
// space.
//
// The popup will be flush with the anchor, either above or below it, and it
// will be horizontally aligned to either the start or the end of the anchor.
// This gives four candidate placements: below-start, below-end, above-start,
// and above-end. The one that covers the most of `preferred_size` wins. (Ties
// are broken according to the order above.)
//
// A non-positive component of `min_size` means no minimum size on that axis.
//
alia_box
alia_popup_place(
    alia_box anchor,
    alia_vec2f preferred_size,
    alia_vec2f min_size,
    alia_box bounds);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_POPUP_H
