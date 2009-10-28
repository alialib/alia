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

// The following flags are used to specify various aspects of a widget's
// layout, including how it is placed within the space allocated to it (its
// alignment).
// Omitting these flags invokes the default behavior for the widget, which
// depends on the type of widget.

struct layout_flag_set
{
    unsigned code;
    layout_flag_set() {}
    explicit layout_flag_set(unsigned code) : code(code) {}
    // allows use within if statements without other unintended conversions
    typedef unsigned layout_flag_set::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return code != 0 ? &layout_flag_set::code : 0; }
};
static inline layout_flag_set
operator|(layout_flag_set a, layout_flag_set b)
{ return layout_flag_set(a.code | b.code); }
static inline layout_flag_set&
operator|=(layout_flag_set& a, layout_flag_set b)
{ a.code |= b.code; return a; }
static inline layout_flag_set
operator&(layout_flag_set a, layout_flag_set b)
{ return layout_flag_set(a.code & b.code); }
static inline layout_flag_set&
operator&=(layout_flag_set& a, layout_flag_set b)
{ a.code &= b.code; return a; }
static inline bool
operator==(layout_flag_set a, layout_flag_set b)
{ return a.code == b.code; }

static layout_flag_set const NO_LAYOUT_FLAGS(0x000);

// X alignment flags
static unsigned const CENTER_X_CODE                                 = 0x001;
static layout_flag_set const CENTER_X(CENTER_X_CODE);
static unsigned const LEFT_CODE                                     = 0x002;
static layout_flag_set const LEFT(LEFT_CODE);
static unsigned const RIGHT_CODE                                    = 0x003;
static layout_flag_set const RIGHT(RIGHT_CODE);
static unsigned const FILL_X_CODE                                   = 0x004;
static layout_flag_set const FILL_X(FILL_X_CODE);
// GROW is the same as FILL, but it also sets the proportion to 1 if you don't
// manually override it.
static unsigned const GROW_X_CODE                                   = 0x00c;
static layout_flag_set const GROW_X(GROW_X_CODE);
static unsigned const X_ALIGNMENT_MASK_CODE                         = 0x00f;
static layout_flag_set const X_ALIGNMENT_MASK(X_ALIGNMENT_MASK_CODE);

// Y alignment flags
static unsigned const CENTER_Y_CODE                                 = 0x010;
static layout_flag_set const CENTER_Y(CENTER_Y_CODE);
static unsigned const TOP_CODE                                      = 0x020;
static layout_flag_set const TOP(TOP_CODE);
static unsigned const BOTTOM_CODE                                   = 0x030;
static layout_flag_set const BOTTOM(BOTTOM_CODE);
static unsigned const FILL_Y_CODE                                   = 0x040;
static layout_flag_set const FILL_Y(FILL_Y_CODE);
static unsigned const BASELINE_Y_CODE                               = 0x050;
static layout_flag_set const BASELINE_Y(BASELINE_Y_CODE);
static unsigned const GROW_Y_CODE                                   = 0x0c0;
static layout_flag_set const GROW_Y(GROW_Y_CODE);
static unsigned const Y_ALIGNMENT_MASK_CODE                         = 0x0f0;
static layout_flag_set const Y_ALIGNMENT_MASK(Y_ALIGNMENT_MASK_CODE);

// combined alignment flags
static layout_flag_set const CENTER(CENTER_X_CODE | CENTER_Y_CODE);
static layout_flag_set const FILL(FILL_X_CODE | FILL_Y_CODE);
static layout_flag_set const GROW(GROW_X_CODE | GROW_Y_CODE);
// PROPORTIONAL_FILL and PROPORTIONAL_GROW are like FILL and GROW, but the
// width and height are constrained to their original ratio.  These should
// only be used for leaf widgets, not containers.
static unsigned const PROPORTIONAL_FILL_CODE                        = 0x077;
static layout_flag_set const PROPORTIONAL_FILL(PROPORTIONAL_FILL_CODE);
static unsigned const PROPORTIONAL_GROW_CODE                        = 0x0ff;
static layout_flag_set const PROPORTIONAL_GROW(PROPORTIONAL_GROW_CODE);
static unsigned const ALIGNMENT_MASK_CODE                           = 0x0ff;
static layout_flag_set const ALIGNMENT_MASK(ALIGNMENT_MASK_CODE);

// other flags
// explicitly enabled or disable the padding around a widget
static unsigned const PADDED_CODE                                   = 0x100;
static layout_flag_set const PADDED(PADDED_CODE);
static unsigned const NOT_PADDED_CODE                               = 0x200;
static layout_flag_set const NOT_PADDED(NOT_PADDED_CODE);

// All widgets accept a layout structure as a parameter.  It allows the user
// to control some aspects of the widget's layout.
//
struct layout
{
    layout(alia::size const& size = alia::size(-1, -1, PIXELS),
        layout_flag_set flags = NO_LAYOUT_FLAGS, float proportion = 0)
      : size(size)
      , flags(flags)
      , proportion(proportion)
    {}
    layout(layout_flag_set flags, float proportion = 0)
      : size(-1, -1, PIXELS)
      , flags(flags)
      , proportion(proportion)
    {}
    alia::size size;
    layout_flag_set flags;
    float proportion;
};
extern layout default_layout;

bool operator==(layout const& a, layout const& b);
bool operator!=(layout const& a, layout const& b);

}

#endif
