#ifndef ALIA_UI_LAYOUT_SPECIFICATION_HPP
#define ALIA_UI_LAYOUT_SPECIFICATION_HPP

#include <alia/ui/geometry.hpp>

// This file defines the types and flags available to the application to
// specify layout properties.

namespace alia {

typedef float layout_scalar;

typedef vector<2, layout_scalar> layout_vector;

inline layout_vector
make_layout_vector(layout_scalar x, layout_scalar y)
{
    return make_vector(x, y);
}

typedef box<2, layout_scalar> layout_box;

typedef layout_scalar absolute_length;

typedef vector<2, layout_scalar> absolute_size;

// box_border_width specifies the width of the borders of a box.
// Each side can have a different border width.
template<class Scalar>
struct box_border_width
{
    Scalar top, right, bottom, left;

    auto
    operator<=>(box_border_width const&) const
        = default;
};

// some convenience functions for specifying widget sizes
inline absolute_size
size(absolute_length w, absolute_length h)
{
    return make_vector(w, h);
}
// These only specify a single dimension.
// Note that setting the other dimension to 0 is harmless as these are
// specifying a minimum size.
inline absolute_size
width(absolute_length w)
{
    return make_vector(w, 0.f);
}
inline absolute_size
height(absolute_length h)
{
    return make_vector(0.f, h);
}

// The following flags are used to specify various aspects of an element's
// layout, including how it is placed within the space allocated to it (its
// alignment).
// Omitting these flags invokes the default behavior for the element, which
// depends on the type of element.

ALIA_DEFINE_FLAG_TYPE(layout)

// X alignment flags
ALIA_DEFINE_FLAG(layout, 0x0001, CENTER_X)
ALIA_DEFINE_FLAG(layout, 0x0002, LEFT)
ALIA_DEFINE_FLAG(layout, 0x0003, RIGHT)
ALIA_DEFINE_FLAG(layout, 0x0004, FILL_X)
// Note that there's currently no baseline for the X axis, but the code is
// defined here anyway for use in the Y section.
ALIA_DEFINE_FLAG(layout, 0x0005, BASELINE_X)
// GROW is the same as FILL, but it also sets the growth_factor to 1 if you
// don't manually override it.
// Note that GROW currently has its own bit for easier testing.
ALIA_DEFINE_FLAG(layout, 0x0008, GROW_X)
ALIA_DEFINE_FLAG(layout, 0x000f, X_ALIGNMENT_MASK)

// Y alignment flags - Note that the Y codes are the same as the X codes but
// shifted left by X_TO_Y_SHIFT bits.
unsigned const X_TO_Y_SHIFT = 4;
ALIA_DEFINE_FLAG(layout, CENTER_X_CODE << X_TO_Y_SHIFT, CENTER_Y)
ALIA_DEFINE_FLAG(layout, LEFT_CODE << X_TO_Y_SHIFT, TOP)
ALIA_DEFINE_FLAG(layout, RIGHT_CODE << X_TO_Y_SHIFT, BOTTOM)
ALIA_DEFINE_FLAG(layout, FILL_X_CODE << X_TO_Y_SHIFT, FILL_Y)
ALIA_DEFINE_FLAG(layout, BASELINE_X_CODE << X_TO_Y_SHIFT, BASELINE_Y)
ALIA_DEFINE_FLAG(layout, GROW_X_CODE << X_TO_Y_SHIFT, GROW_Y)
ALIA_DEFINE_FLAG(
    layout, X_ALIGNMENT_MASK_CODE << X_TO_Y_SHIFT, Y_ALIGNMENT_MASK)

// combined alignment flags - These specify both X and Y simultaneously.
layout_flag_set const CENTER = CENTER_X | CENTER_Y;
layout_flag_set const FILL = FILL_X | FILL_Y;
layout_flag_set const GROW = GROW_X | GROW_Y;
// PROPORTIONAL_FILL and PROPORTIONAL_GROW are like FILL and GROW, but the
// width and height are constrained to their original ratio. These should
// only be used for leaf element, not containers.
ALIA_DEFINE_FLAG(layout, 0x0100, PROPORTIONAL_FILL)
ALIA_DEFINE_FLAG(layout, 0x0200, PROPORTIONAL_GROW)

// explicitly enable or disable the padding around an element
ALIA_DEFINE_FLAG(layout, 0x1000, PADDED)
ALIA_DEFINE_FLAG(layout, 0x2000, UNPADDED)
ALIA_DEFINE_FLAG(layout, 0x3000, PADDING_MASK)

// This is the main structure used to control an element's layout.
struct layout
{
    layout(
        absolute_size const& size = alia::size(0, 0),
        layout_flag_set flags = NO_FLAGS,
        float growth_factor = 0)
        : size(size), flags(flags), growth_factor(growth_factor)
    {
    }

    layout(layout_flag_set flags, float growth_factor = 0)
        : size(alia::size(0, 0)), flags(flags), growth_factor(growth_factor)
    {
    }

    absolute_size size;
    layout_flag_set flags;
    float growth_factor;

    // auto
    // operator<=>(layout const&) const
    //     = default;
};

layout const default_layout;

} // namespace alia

#endif
