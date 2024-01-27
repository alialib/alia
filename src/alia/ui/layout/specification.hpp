#ifndef ALIA_INDIE_LAYOUT_SPECIFICATION_HPP
#define ALIA_INDIE_LAYOUT_SPECIFICATION_HPP

#include <alia/indie/geometry.hpp>

// This file defines the types and flags available to the application to
// specify layout properties.

namespace alia { namespace indie {

typedef float layout_scalar;

typedef vector<2, layout_scalar> layout_vector;
inline layout_vector
make_layout_vector(layout_scalar x, layout_scalar y)
{
    return make_vector(x, y);
}

typedef box<2, layout_scalar> layout_box;

enum layout_units
{
    PIXELS,

    // won't be affected by the global magnification factor
    UNMAGNIFIED_PIXELS,

    // physical units
    INCHES,
    CM,
    MM,
    POINT, // point - 1/72 of an inch
    PICA, // pica - 12 points

    // character cells - One character cell is equal to the size of an average
    // character in the current font, including ascent and descent.
    // Unlike other units, CHARS has a different interpretation for X and Y
    // (i.e., 1 CHAR x 1 CHAR is probably not a square).
    CHARS,

    // borrowed from CSS - One EM is equal to the current font size.
    // One EX is equal to the height of the character 'x' in the current font.
    EM,
    EX,
};

// Absolute lengths are used to specify the size of stand-alone widgets.
struct absolute_length
{
    float length;
    layout_units units;

    absolute_length(float length, layout_units units)
        : length(length), units(units)
    {
    }
    absolute_length()
    {
    }
};
inline bool
operator==(absolute_length const& a, absolute_length const& b)
{
    return a.length == b.length && a.units == b.units;
}
inline bool
operator!=(absolute_length const& a, absolute_length const& b)
{
    return !(a == b);
}

// 2D version of absolute_length
typedef vector<2, absolute_length> absolute_size;

// Relative lengths is used to sepcify the size of widget components.
// They can be either be specified in absolute units or as a fraction of the
// full widget size.
struct relative_length
{
    bool is_relative;
    float length;
    layout_units units; // only relevant if is_relative is false

    relative_length(float length, layout_units units)
        : is_relative(false), length(length), units(units)
    {
    }
    relative_length(float length) : is_relative(true), length(length)
    {
    }
    relative_length()
    {
    }
};
inline bool
operator==(relative_length const& a, relative_length const& b)
{
    return a.length == b.length && a.is_relative == b.is_relative
           && (!a.is_relative || a.units == b.units);
}
inline bool
operator!=(relative_length const& a, relative_length const& b)
{
    return !(a == b);
}

// 2D version of relative_length
typedef vector<2, relative_length> relative_size;

// box_border_width specifies the width of the borders of a box.
// Each side can have a different border width.
template<class Scalar>
struct box_border_width
{
    Scalar top, right, bottom, left;
    box_border_width()
    {
    }
    box_border_width(Scalar top, Scalar right, Scalar bottom, Scalar left)
        : top(top), right(right), bottom(bottom), left(left)
    {
    }
    explicit box_border_width(Scalar width)
        : top(width), right(width), bottom(width), left(width)
    {
    }
};
template<class Scalar>
bool
operator==(
    box_border_width<Scalar> const& a, box_border_width<Scalar> const& b)
{
    return a.top == b.top && a.right == b.right && a.bottom == b.bottom
           && a.left == b.left;
}
template<class Scalar>
bool
operator!=(
    box_border_width<Scalar> const& a, box_border_width<Scalar> const& b)
{
    return !(a == b);
}

// some convenience functions for specifying widget sizes
inline absolute_size
size(float w, float h, layout_units u)
{
    return make_vector(absolute_length(w, u), absolute_length(h, u));
}
inline absolute_size
size(float w, layout_units wu, float h, layout_units hu)
{
    return make_vector(absolute_length(w, wu), absolute_length(h, hu));
}
// These only specify a single dimension.
// Note that setting the other dimension to 0 is harmless as these are
// specifying a minimum size.
inline absolute_size
width(float w, layout_units u)
{
    return make_vector(absolute_length(w, u), absolute_length(0, PIXELS));
}
inline absolute_size
height(float h, layout_units u)
{
    return make_vector(absolute_length(0, PIXELS), absolute_length(h, u));
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
        absolute_size const& size = indie::size(0, 0, PIXELS),
        layout_flag_set flags = NO_FLAGS,
        float growth_factor = 0)
        : size(size), flags(flags), growth_factor(growth_factor)
    {
    }
    layout(layout_flag_set flags, float growth_factor = 0)
        : size(indie::size(0, 0, PIXELS)),
          flags(flags),
          growth_factor(growth_factor)
    {
    }
    absolute_size size;
    layout_flag_set flags;
    float growth_factor;
};
inline bool
operator==(layout const& a, layout const& b)
{
    return a.size == b.size && a.flags == b.flags
           && a.growth_factor == b.growth_factor;
}
inline bool
operator!=(layout const& a, layout const& b)
{
    return !(a == b);
}

layout const default_layout;

}} // namespace alia::indie

#endif
