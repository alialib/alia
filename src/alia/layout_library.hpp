#ifndef ALIA_LAYOUT_LIBRARY_HPP
#define ALIA_LAYOUT_LIBRARY_HPP

#include <alia/layout_interface.hpp>

// This file defines a library of standard layout containers and leaves.

namespace alia {

struct simple_layout_container;

// A spacer simply fills space in a layout.
void do_spacer(layout_traversal& traversal,
    layout const& layout_spec = default_layout);
template<class Context>
void do_spacer(Context& ctx, layout const& layout_spec = default_layout)
{ do_spacer(get_layout_traversal(ctx), layout_spec); }

// This version of the spacer records the region that's assigned to it.
void do_spacer(layout_traversal& traversal, layout_box* region,
    layout const& layout_spec = default_layout);
template<class Context>
void do_spacer(Context& ctx, layout_box* region,
    layout const& layout_spec = default_layout)
{ do_spacer(get_layout_traversal(ctx), region, layout_spec); }

layout_box get_container_region(simple_layout_container const& container);

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
        { concrete_begin(get_layout_traversal(ctx), layout_spec); } \
        \
        void end() \
        { \
            if (container_) \
            { transform_.end(); slc_.end(); container_ = 0; } \
        } \
        layout_box region() const \
        { return get_container_region(*container_); } \
        \
     private: \
        void concrete_begin( \
            layout_traversal& traversal, layout const& layout_spec); \
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
        { concrete_begin(get_layout_traversal(ctx), arg, layout_spec); } \
        \
        void end() { transform_.end(); slc_.end(); } \
        \
        layout_box region() const \
        { return get_container_region(*container_); } \
        \
     private: \
        void concrete_begin( \
            layout_traversal& traversal, Arg arg, layout const& layout_spec); \
        \
        simple_layout_container* container_; \
        scoped_layout_container slc_; \
        scoped_transformation transform_; \
    };

// A row layout places all its children in a horizontal row.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(row_layout)

// A column layout places all its children in a vertical column.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(column_layout)

// Linear layout places its children in a line along the specified axis.
// In other words, it's a row if axis is 0 and a column if axis is 1.
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(linear_layout, unsigned)

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

#define ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid) \
    struct grid##_data; \
    struct grid##_layout \
    { \
        grid##_layout() {} \
        \
        template<class Context> \
        grid##_layout( \
            Context& ctx, \
            layout const& layout_spec = default_layout, \
            float column_spacing = 0, \
            layout_units spacing_units = PIXELS) \
        { begin(ctx, layout_spec, column_spacing, spacing_units); } \
        \
        ~grid##_layout() { end(); } \
        \
        template<class Context> \
        void begin( \
            Context& ctx, \
            layout const& layout_spec = default_layout, \
            float column_spacing = 0, \
            layout_units spacing_units = PIXELS) \
        { \
            concrete_begin(get_layout_traversal(ctx), layout_spec, \
                column_spacing, spacing_units); \
        } \
        \
        void end() { transform_.end(); container_.end(); } \
    \
     private: \
        void concrete_begin( \
            layout_traversal& traversal, layout const& layout_spec, \
            float column_spacing, layout_units spacing_units); \
        \
        friend struct grid##_row; \
        \
        scoped_layout_container container_; \
        scoped_transformation transform_; \
        layout_traversal* traversal_; \
        grid##_data* data_; \
    }; \
    struct grid##_row \
    { \
        grid##_row() {} \
        grid##_row(grid##_layout const& g, \
            layout const& layout_spec = default_layout) \
        { begin(g, layout_spec); } \
        ~grid##_row() { end(); } \
        void begin(grid##_layout const& g, \
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
ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid)

// A uniform grid layout is similar to a grid layout but all cells in the grid
// always have a uniform size.
ALIA_DECLARE_GRID_LAYOUT_CONTAINER(uniform_grid)

// A floating_layout detaches its contents from the parent context.
// The layout should only have one child (generally another container).
// This child becomes the root of an independent layout tree.
// The caller may specify the maximum size of the child.
// Note that the child is simply placed at (0, 0).
// It's assumed that the caller will take care of positioning it properly.
struct floating_layout_data;
struct floating_layout
{
    floating_layout() : traversal_(0) {}

    template<class Context>
    floating_layout(Context& ctx, layout_vector const& max_size)
    { begin(ctx, position max_size); }

    ~floating_layout() { end(); }

    template<class Context>
    void begin(Context& ctx, layout_vector const& max_size)
    { concrete_begin(get_layout_traversal(ctx), max_size); }

    void end();

    layout_vector size() const;

    layout_box region() const
    { return layout_box(make_layout_vector(0, 0), size()); }

 private:
    void concrete_begin(
        layout_traversal& traversal,
        layout_vector const& max_size);

    layout_traversal* traversal_;
    layout_container* old_container_;
    layout_node** old_next_ptr_;
    floating_layout_data* data_;
    scoped_clip_region_reset clipping_reset_;
};

}

#endif
