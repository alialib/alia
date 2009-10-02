#ifndef ALIA_LAYOUT_INTERFACE_HPP
#define ALIA_LAYOUT_INTERFACE_HPP

#include <alia/forward.hpp>
#include <alia/vector.hpp>

namespace alia {

#ifdef CM
  #undef CM
#endif
#ifdef MM
  #undef MM
#endif

enum unit_t
{
    PIXELS,

    // physical units
    INCHES, CM, MM,

    // character cells - One character cell is equal to the size of an average
    // character in the current font, including ascent and descent.
    // Unlike other units, CHARS has a different interpretation for X and Y
    // (i.e., 1 CHAR x 1 CHAR is probably not a square).
    CHARS,

    // borrowed from CSS - One EM is equal to the current font size.
    EM,
};

struct size
{
    size(float w, float h, unit_t u)
      : width(w), height(h), x_units(u), y_units(u) {}
    size(float w, unit_t xu, float h, unit_t yu)
      : width(w), height(h), x_units(xu), y_units(yu) {}
    size() {}
    float width, height;
    unit_t x_units, y_units;
};

bool operator==(size const& a, size const& b);
bool operator!=(size const& a, size const& b);

static inline size width(float w, unit_t u) { return size(w, u, -1, PIXELS); }
static inline size height(float h, unit_t u) { return size(-1, PIXELS, h, u); }

// All widgets accept a layout structure as a parameter.  It allows the user
// to control some aspects of the widget's layout.
//
struct layout
{
    layout(alia::size const& size = alia::size(-1, -1, PIXELS),
        unsigned flags = 0, float proportion = 0)
      : size(size)
      , flags(flags)
      , proportion(proportion)
    {}
    layout(unsigned flags, float proportion = 0)
      : size(-1, -1, PIXELS)
      , flags(flags)
      , proportion(proportion)
    {}
    alia::size size;
    unsigned flags;
    float proportion;
};
extern layout default_layout;

bool operator==(layout const& a, layout const& b);
bool operator!=(layout const& a, layout const& b);

// The following flags are used to specify various aspects of a widget's
// layout, including how it is placed within the space allocated to it (its
// alignment).
// Omitting these flags invokes the default behavior for the widget, which
// depends on the type of widget.

// X alignment flags
static unsigned const CENTER_X            = 0x001;
static unsigned const LEFT                = 0x002;
static unsigned const RIGHT               = 0x003;
static unsigned const FILL_X              = 0x004;
// GROW is the same as FILL, but it also sets the proportion to 1 if you don't
// manually override it.
static unsigned const GROW_X              = 0x00c;
static unsigned const X_ALIGNMENT_MASK    = 0x00f;

// Y alignment flags
static unsigned const CENTER_Y            = 0x010;
static unsigned const TOP                 = 0x020;
static unsigned const BOTTOM              = 0x030;
static unsigned const FILL_Y              = 0x040;
static unsigned const BASELINE_Y          = 0x050;
static unsigned const GROW_Y              = 0x0c0;
static unsigned const Y_ALIGNMENT_MASK    = 0x0f0;

// combined alignment flags
static unsigned const CENTER              = CENTER_X | CENTER_Y;
static unsigned const FILL                = FILL_X | FILL_Y;
static unsigned const GROW                = GROW_X | GROW_Y;
// PROPORTIONAL_FILL and PROPORTIONAL_GROW are like FILL and GROW, but the
// width and height are constrained to their original ratio.  These should
// only be used for leaf widgets, not containers.
static unsigned const PROPORTIONAL_FILL   = 0x077;
static unsigned const PROPORTIONAL_GROW   = 0x0ff;
static unsigned const ALIGNMENT_MASK      = 0x0ff;

// other flags
// explicitly enabled or disable the padding around a widget
static unsigned const PADDED              = 0x100;
static unsigned const NOT_PADDED          = 0x200;

}

#endif
