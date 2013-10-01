#ifndef ALIA_LAYOUT_API_HPP
#define ALIA_LAYOUT_API_HPP

#include <alia/geometry.hpp>

// This file defines all types and functions necessary for interfacing with the
// layout system from the application end, including a library of standard
// containers.

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

    // won't be affected by the global magnification factor
    UNMAGNIFIED_PIXELS,

    // physical units
    INCHES, CM, MM,
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
    {}
    absolute_length() {}
};
bool operator==(absolute_length const& a, absolute_length const& b);
bool operator!=(absolute_length const& a, absolute_length const& b);

// 2D version of absolute_length
typedef vector<2,absolute_length> absolute_size;

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
    {}
    relative_length(float length)
      : is_relative(true), length(length)
    {}
    relative_length() {}
};
bool operator==(relative_length const& a, relative_length const& b);
bool operator!=(relative_length const& a, relative_length const& b);

// 2D version of relative_length
typedef vector<2,relative_length> relative_size;

// box_border_width specifies the width of the borders of a box.
// Each side can have a different border width.
template<class Scalar>
struct box_border_width
{
    Scalar top, right, bottom, left;
    box_border_width() {}
    box_border_width(
        Scalar top, Scalar right, Scalar bottom, Scalar left)
      : top(top), right(right), bottom(bottom), left(left)
    {}
    explicit box_border_width(Scalar width)
      : top(width), right(width), bottom(width), left(width)
    {}
};
template<class Scalar>
bool operator==(box_border_width<Scalar> const& a,
    box_border_width<Scalar> const& b)
{
    return a.top == b.top && a.right == b.right &&
        a.bottom == b.bottom && a.left == b.left;
}
template<class Scalar>
bool operator!=(box_border_width<Scalar> const& a,
    box_border_width<Scalar> const& b)
{
    return !(a == b);
}

// some convenience functions for specifying widget sizes
static inline absolute_size
size(float w, float h, layout_units u)
{ return make_vector(absolute_length(w, u), absolute_length(h, u)); }
static inline absolute_size
size(float w, layout_units wu, float h, layout_units hu)
{ return make_vector(absolute_length(w, wu), absolute_length(h, hu)); }
// These only specify a single dimension.
// Note that setting the other dimension to 0 is harmless as these are
// specifying a minimum size.
static inline absolute_size
width(float w, layout_units u)
{ return make_vector(absolute_length(w, u), absolute_length(0, PIXELS)); }
static inline absolute_size
height(float h, layout_units u)
{ return make_vector(absolute_length(0, PIXELS), absolute_length(h, u)); }

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
static unsigned const X_TO_Y_SHIFT = 4;
ALIA_DEFINE_FLAG(layout, CENTER_X_CODE << X_TO_Y_SHIFT, CENTER_Y)
ALIA_DEFINE_FLAG(layout, LEFT_CODE << X_TO_Y_SHIFT, TOP)
ALIA_DEFINE_FLAG(layout, RIGHT_CODE << X_TO_Y_SHIFT, BOTTOM)
ALIA_DEFINE_FLAG(layout, FILL_X_CODE << X_TO_Y_SHIFT, FILL_Y)
ALIA_DEFINE_FLAG(layout, BASELINE_X_CODE << X_TO_Y_SHIFT,
    BASELINE_Y)
ALIA_DEFINE_FLAG(layout, GROW_X_CODE << X_TO_Y_SHIFT, GROW_Y)
ALIA_DEFINE_FLAG(layout, X_ALIGNMENT_MASK_CODE << X_TO_Y_SHIFT,
    Y_ALIGNMENT_MASK)

// combined alignment flags - These specify both X and Y simultaneously.
static layout_flag_set const CENTER = CENTER_X | CENTER_Y;
static layout_flag_set const FILL = FILL_X | FILL_Y;
static layout_flag_set const GROW = GROW_X | GROW_Y;
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
    layout(absolute_size const& size = alia::size(0, 0, PIXELS),
        layout_flag_set flags = NO_FLAGS, float growth_factor = 0)
      : size(size)
      , flags(flags)
      , growth_factor(growth_factor)
    {}
    layout(layout_flag_set flags, float growth_factor = 0)
      : size(alia::size(0, 0, PIXELS))
      , flags(flags)
      , growth_factor(growth_factor)
    {}
    absolute_size size;
    layout_flag_set flags;
    float growth_factor;
};
bool operator==(layout const& a, layout const& b);
bool operator!=(layout const& a, layout const& b);

static layout const default_layout;

struct data_traversal;
struct geometry_context;

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

// GEOMETRY CONTEXT

// geometry_context represents a context for defining 2D geometry.
// It provides a transformation matrix, which maps the current frame of
// reference for the context to its root frame of reference.
// It also provides a clip region, which is a rectangle defined in the root
// frame of reference.
// An object may 'subscribe' to a geometry_context so that it is informed of
// changes in the state of the context.
struct geometry_context_subscriber
{
    virtual void set_transformation_matrix(
        matrix<3,3,double> const& transformation_matrix) = 0;
    virtual void set_clip_region(
        box<2,double> const& clip_region) = 0;
};
struct geometry_context
{
    box<2,double> full_region;
    matrix<3,3,double> transformation_matrix;
    box<2,double> clip_region;
    geometry_context_subscriber* subscriber;
};

// geometry_contexts are specified by the same generic Context concept as is
// used in data_graph.hpp.
static inline geometry_context& get_geometry_context(geometry_context& ctx)
{ return ctx; }

// scoped_clip_region is a scoped object that further restricts the clip region
// of a geometry_context while it's active. While it's active, the context's
// clip region will be set to the intersection of the old region and what's
// passed to set().
// The region is specified in the current frame of reference for the context.
// However, since the region for the context must be an axis-aligned rectangle
// in the root frame of reference, this will only work properly if the current
// transformation matrix is composed only of translations, scaling, and
// 90-degree rotations.
class scoped_clip_region : noncopyable
{
 public:
    scoped_clip_region() : ctx_(0) {}
    template<class Context>
    scoped_clip_region(Context& ctx)
    { begin(get_geometry_context(ctx)); }
    template<class Context>
    scoped_clip_region(Context& ctx, box<2,double> const& region)
    { begin(ctx); set(region); }
    ~scoped_clip_region() { end(); }
    void begin(geometry_context& ctx);
    void set(box<2,double> const& region);
    void restore();
    void end();
 private:
    geometry_context* ctx_;
    box<2,double> old_region_;
};

// scoped_clip_region_resets the clip region to the full geometry region for
// its scope. This can be useful for drawing overlays which are meant to
// extend beyond the clip region normally associated with their scope.
class scoped_clip_region_reset : noncopyable
{
 public:
    scoped_clip_region_reset() : ctx_(0) {}
    template<class Context>
    scoped_clip_region_reset(Context& ctx)
    { begin(get_geometry_context(ctx)); }
    ~scoped_clip_region_reset() { end(); }
    void begin(geometry_context& ctx);
    void end();
 private:
    geometry_context* ctx_;
    box<2,double> old_region_;
};

// scoped_transformation is a scoped object that applies a transformation to a
// geometry_context. While it's active, the context's transformation matrix
// will be set to the product of the old matrix and what's passed to set().
class scoped_transformation : noncopyable
{
 public:
    scoped_transformation() : ctx_(0) {}
    template<class Context>
    scoped_transformation(Context& ctx)
    { begin(get_geometry_context(ctx)); }
    template<class Context>
    scoped_transformation(Context& ctx,
        matrix<3,3,double> const& transformation)
    { begin(get_geometry_context(ctx)); set(transformation); }
    ~scoped_transformation() { end(); }
    void begin(geometry_context& ctx);
    void set(matrix<3,3,double> const& transformation);
    void restore();
    void end();
 private:
    geometry_context* ctx_;
    matrix<3,3,double> old_matrix_;
};

// LIBRARY

// A spacer simply fills space in a layout.
void do_spacer(layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec = default_layout);
// context-based interface
template<class Context>
void do_spacer(Context& ctx, layout const& layout_spec = default_layout)
{
    do_spacer(get_layout_traversal(ctx), get_data_traversal(ctx),
        layout_spec);
}

// This version of the spacer records the region that's assigned to it.
void do_spacer(layout_traversal& traversal, data_traversal& data,
    layout_box* region, layout const& layout_spec = default_layout);
// context-based interface
template<class Context>
void do_spacer(Context& ctx, layout_box* region,
    layout const& layout_spec = default_layout)
{
    do_spacer(get_layout_traversal(ctx), get_data_traversal(ctx),
        region, layout_spec);
}

layout_box get_container_region(simple_layout_container const& container);
layout_box get_padded_container_region(
    simple_layout_container const& container);
layout_vector get_container_offset(simple_layout_container const& container);

#define ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(container_name) \
    struct container_name \
    { \
        container_name() : container_(0) {} \
        \
        template<class Context> \
        container_name(Context& ctx, \
            layout const& layout_spec = default_layout) \
        { begin(ctx, layout_spec); } \
        \
        ~container_name() { end(); } \
        \
        template<class Context> \
        void begin(Context& ctx, layout const& layout_spec = default_layout) \
        { concrete_begin(get_layout_traversal(ctx), \
            get_data_traversal(ctx), layout_spec); } \
        \
        void end() \
        { \
            if (container_) \
            { transform_.end(); slc_.end(); container_ = 0; } \
        } \
        \
        layout_box region() const \
        { return get_container_region(*container_); } \
        \
        layout_box padded_region() const \
        { return get_padded_container_region(*container_); } \
        \
        layout_vector offset() const \
        { return get_container_offset(*container_); } \
        \
     private: \
        void concrete_begin( \
            layout_traversal& traversal, data_traversal& data, \
            layout const& layout_spec); \
        \
        simple_layout_container* container_; \
        scoped_layout_container slc_; \
        scoped_transformation transform_; \
    };

#define ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(container_name, Arg) \
    struct container_name \
    { \
        container_name() {} \
        \
        template<class Context> \
        container_name(Context& ctx, Arg arg, \
            layout const& layout_spec = default_layout) \
        { begin(ctx, arg, layout_spec); } \
        \
        ~container_name() { end(); } \
        \
        template<class Context> \
        void begin(Context& ctx, Arg arg, \
            layout const& layout_spec = default_layout) \
        { concrete_begin(get_layout_traversal(ctx), get_data_traversal(ctx), \
            arg, layout_spec); } \
        \
        void end() { transform_.end(); slc_.end(); } \
        \
        layout_box region() const \
        { return get_container_region(*container_); } \
        \
        layout_box padded_region() const \
        { return get_padded_container_region(*container_); } \
        \
        layout_vector offset() const \
        { return get_container_offset(*container_); } \
        \
     private: \
        void concrete_begin(layout_traversal& traversal, \
            data_traversal& data, Arg arg, layout const& layout_spec); \
        \
        simple_layout_container* container_; \
        scoped_layout_container slc_; \
        scoped_transformation transform_; \
    };

// A row layout places all its children in a horizontal row.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(row_layout)

// A column layout places all its children in a vertical column.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(column_layout)

// Linear layout places its children in a line.
// In other words, it can act as either a row or a column depending on the
// argument you give it.
ALIA_DEFINE_FLAG_TYPE(linear_layout)
ALIA_DEFINE_FLAG(linear_layout, 0, HORIZONTAL_LAYOUT)
ALIA_DEFINE_FLAG(linear_layout, 1, VERTICAL_LAYOUT)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(
    linear_layout, linear_layout_flag_set)

// Layered layout places all its children in the same rectangle, so they are
// in effect layered over the same region.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(layered_layout)

// A rotated layout rotates its child 90 degrees counterclockwise.
// Note that a rotated layout should only have one child. If it has multiple
// children, it will simply layer them.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(rotated_layout)

// A flow layout arranges its children in horizontal rows, wrapping them around
// to new rows as needed.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(flow_layout)

// A vertical flow layout arranges its children in columns. Widgets flow down
// the columns, starting with the left column. Note that like all containers,
// this one is still primarily driven by the horizontal space allocated to it,
// so it will prefer to create many short columns rather than one long one.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(vertical_flow_layout)

// clamped_layout imposes a maximum size on its child.
// (It should only have a single child.)
// If the space assigned to it is larger than the specified maximum, it
// centers the child within that space.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(clamped_layout, absolute_size)

// bordered_layout adds a border to its child.
// (It should only have a single child.)
// (Note that the scalar type should ideally be relative_length, but that's a
// little more complicated to implement.)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(
    bordered_layout, box_border_width<absolute_length>)

// A clip_evasion_layout will move its child around inside its assigned region
// to try to keep the child visible.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(clip_evasion_layout)

#define ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid_type, row_type) \
    struct grid_type##_data; \
    struct grid_type \
    { \
        grid_type() {} \
        \
        template<class Context> \
        grid_type( \
            Context& ctx, \
            layout const& layout_spec = default_layout, \
            absolute_length const& column_spacing = \
                absolute_length(0, PIXELS)) \
        { begin(ctx, layout_spec, column_spacing); } \
        \
        ~grid_type() { end(); } \
        \
        template<class Context> \
        void begin( \
            Context& ctx, \
            layout const& layout_spec = default_layout, \
            absolute_length const& column_spacing = \
                absolute_length(0, PIXELS)) \
        { \
            concrete_begin(get_layout_traversal(ctx), \
                get_data_traversal(ctx), layout_spec, column_spacing); \
        } \
        \
        void end() { transform_.end(); container_.end(); } \
    \
     private: \
        void concrete_begin(layout_traversal& traversal, \
            data_traversal& data, layout const& layout_spec, \
            absolute_length const& column_spacing); \
        \
        friend struct row_type; \
        \
        scoped_layout_container container_; \
        scoped_transformation transform_; \
        layout_traversal* traversal_; \
        data_traversal* data_traversal_; \
        grid_type##_data* data_; \
    }; \
    struct row_type \
    { \
        row_type() {} \
        row_type(grid_type const& g, \
            layout const& layout_spec = default_layout) \
        { begin(g, layout_spec); } \
        ~row_type() { end(); } \
        void begin(grid_type const& g, \
            layout const& layout_spec = default_layout); \
        void end(); \
     private: \
        scoped_layout_container container_; \
        scoped_transformation transform_; \
    };

// A grid layout is used to arrange widgets in a grid. To use it, create
// grid_row containers that reference the grid_layout.
// Note that the grid_layout container by itself is just a normal column, so
// you can intersperse other widgets amongst the grid rows.
ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid_layout, grid_row)

// A uniform grid layout is similar to a grid layout but all cells in the grid
// have a uniform size.
ALIA_DECLARE_GRID_LAYOUT_CONTAINER(uniform_grid_layout, uniform_grid_row)

// A floating_layout detaches its contents from the parent context.
// It should only have one child (generally another container).
// This child becomes the root of an independent layout tree.
// The caller may specify the minimum and maximum size of the child.
// Note that the child is simply placed at (0, 0).
// It's assumed that the caller will take care of positioning it properly.
struct floating_layout_data;
struct floating_layout
{
    floating_layout() : traversal_(0) {}

    template<class Context>
    floating_layout(Context& ctx,
        layout_vector const& min_size = make_layout_vector(-1, -1),
        layout_vector const& max_size = make_layout_vector(-1, -1))
    { begin(ctx, min_size, max_size); }

    ~floating_layout() { end(); }

    template<class Context>
    void begin(Context& ctx,
        layout_vector const& min_size = make_layout_vector(-1, -1),
        layout_vector const& max_size = make_layout_vector(-1, -1))
    {
        concrete_begin(get_layout_traversal(ctx), get_data_traversal(ctx),
            min_size, max_size);
    }

    void end();

    layout_vector size() const;

    layout_box region() const
    { return layout_box(make_layout_vector(0, 0), size()); }

 private:
    void concrete_begin(layout_traversal& traversal, data_traversal& data,
        layout_vector const& min_size, layout_vector const& max_size);

    layout_traversal* traversal_;
    layout_container* old_container_;
    layout_node** old_next_ptr_;
    layout_node* floating_root_;
    floating_layout_data* data_;
    scoped_clip_region_reset clipping_reset_;
    layout_vector min_size_, max_size_;
};

}

#endif
