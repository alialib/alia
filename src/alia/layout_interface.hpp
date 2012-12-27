#ifndef ALIA_LAYOUT_INTERFACE_HPP
#define ALIA_LAYOUT_INTERFACE_HPP

#include <alia/geometry.hpp>

// This file defines all types and functions necessary for interfacing with the
// layout system from the application end.

namespace alia {

// All positioning is currently done in integer coordinates, but these
// typedefs are used to keep open the possibility of changing that one day.

typedef int layout_scalar;

typedef vector<2,layout_scalar> layout_vector;
static inline layout_vector
make_layout_vector(layout_scalar x, layout_scalar y)
{ return make_vector(x, y); }

typedef box<2,layout_scalar> layout_box;

enum layout_units
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
    // One EX is equal to the height of the character 'x' in the current font.
    EM,
    EX
};

// This is used by the application to specify the size of a layout element.
struct size
{
    size(float w, float h, layout_units u)
      : width(w), height(h), x_units(u), y_units(u) {}
    size(float w, layout_units xu, float h, layout_units yu)
      : width(w), height(h), x_units(xu), y_units(yu) {}
    size() {}
    float width, height;
    layout_units x_units, y_units;
};
bool operator==(size const& a, size const& b);
bool operator!=(size const& a, size const& b);

// If the application only wants to specify one dimension of the element's
// size, these can be used.
static inline size width(float w, layout_units u)
{ return size(w, u, -1, PIXELS); }
static inline size height(float h, layout_units u)
{ return size(-1, PIXELS, h, u); }

// The following flags are used to specify various aspects of an element's
// layout, including how it is placed within the space allocated to it (its
// alignment).
// Omitting these flags invokes the default behavior for the element, which
// depends on the type of element.

struct layout_flag_tag {};
typedef flag_set<layout_flag_tag> layout_flag_set;

// X alignment flags
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0001, CENTER_X)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0002, LEFT)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0003, RIGHT)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0004, FILL_X)
// Note that there's currently no baseline for the X axis, but the code is
// defined here anyway for use in the Y section.
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0005, BASELINE_X)
// GROW is the same as FILL, but it also sets the growth_factor to 1 if you
// don't manually override it.
// Note that GROW currently has its own bit for easier testing.
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0008, GROW_X)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x000f, X_ALIGNMENT_MASK)

// Y alignment flags - Note that the Y codes are the same as the X codes but
// shifted left by X_TO_Y_SHIFT bits.
static unsigned const X_TO_Y_SHIFT = 4;
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, CENTER_X_CODE << X_TO_Y_SHIFT, CENTER_Y)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, LEFT_CODE << X_TO_Y_SHIFT, TOP)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, RIGHT_CODE << X_TO_Y_SHIFT, BOTTOM)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, FILL_X_CODE << X_TO_Y_SHIFT, FILL_Y)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, BASELINE_X_CODE << X_TO_Y_SHIFT,
    BASELINE_Y)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, GROW_X_CODE << X_TO_Y_SHIFT, GROW_Y)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, X_ALIGNMENT_MASK_CODE << X_TO_Y_SHIFT,
    Y_ALIGNMENT_MASK)

// combined alignment flags - These specify both X and Y simultaneously.
static layout_flag_set const CENTER = CENTER_X | CENTER_Y;
static layout_flag_set const FILL = FILL_X | FILL_Y;
static layout_flag_set const GROW = GROW_X | GROW_Y;
// PROPORTIONAL_FILL and PROPORTIONAL_GROW are like FILL and GROW, but the
// width and height are constrained to their original ratio. These should
// only be used for leaf element, not containers.
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0100, PROPORTIONAL_FILL)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x0200, PROPORTIONAL_GROW)

// explicitly enable or disable the padding around an element
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x1000, PADDED)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x2000, UNPADDED)
ALIA_DEFINE_FLAG_CODE(layout_flag_tag, 0x3000, PADDING_MASK)

// This is the main structure used to control an element's layout.
struct layout
{
    layout(alia::size const& size = alia::size(-1, -1, PIXELS),
        layout_flag_set flags = NO_FLAGS, float growth_factor = 0)
      : size(size)
      , flags(flags)
      , growth_factor(growth_factor)
    {}
    layout(layout_flag_set flags, float growth_factor = 0)
      : size(-1, -1, PIXELS)
      , flags(flags)
      , growth_factor(growth_factor)
    {}
    alia::size size;
    layout_flag_set flags;
    float growth_factor;
};
extern layout default_layout;

bool operator==(layout const& a, layout const& b);
bool operator!=(layout const& a, layout const& b);

struct data_traversal;

// forward declarations from layout_system.hpp
struct layout_system;
struct layout_container;
struct layout_node;
struct layout_style_info;
struct simple_layout_container;

// layout_traversal is the state associated with a traversal of the scene
// that the application wants to lay out.
struct layout_traversal
{
    // This is the layout system that this traversal is traversing.
    layout_system* system;

    // This is a data traversal that's used to retrieve data when necessary.
    data_traversal* data;

    layout_container* active_container;

    layout_node** next_ptr;

    // Iff this is true, then this a refresh pass.
    // A refresh pass is used to specify the contents of the layout tree.
    // Other passes utilize the layout information for their own purposes.
    bool is_refresh_pass;

    // This is incremented each refresh pass.
    counter_type refresh_counter;

    // This is the geometry context that the layout system manipulates during
    // non-refresh passes.
    geometry_context* geometry;

    // This is required for resolving layout specs that are specified in
    // physical units or characters.
    vector<2,float> ppi;
    layout_style_info const* style_info;
};

template<class Context>
bool is_refresh_pass(Context& ctx)
{ return get_layout_traversal(ctx).is_refresh_pass; }

// scoped_layout_container makes a layout container active for its scope.
struct scoped_layout_container : noncopyable
{
    scoped_layout_container() : traversal_(0) {}
    scoped_layout_container(
        layout_traversal& traversal, layout_container* container)
    { begin(traversal, container); }
    ~scoped_layout_container() { end(); }
    void begin(layout_traversal& traversal, layout_container* container);
    void end();
 private:
    layout_traversal* traversal_;
};

}

#endif
