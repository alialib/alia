#ifndef ALIA_INDIE_LAYOUT_LIBRARY_HPP
#define ALIA_INDIE_LAYOUT_LIBRARY_HPP

#include <alia/indie/layout/specification.hpp>

namespace alia {

struct data_traversal;

namespace indie {

struct layout_system;
struct widget_container;
struct widget;
struct layout_style_info;
template<class Logic>
struct simple_layout_container;

// scoped_layout_container makes a layout container active for its scope.
struct scoped_layout_container : noncopyable
{
    scoped_layout_container() : traversal_(0)
    {
    }
    scoped_layout_container(
        layout_traversal<widget_container, widget>& traversal,
        widget_container* container)
    {
        begin(traversal, container);
    }
    ~scoped_layout_container()
    {
        end();
    }
    void
    begin(
        layout_traversal<widget_container, widget>& traversal,
        widget_container* container);
    void
    end();

 private:
    layout_traversal<widget_container, widget>* traversal_;
};

// // A spacer simply fills space in a layout.
// void
// do_spacer(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout const& layout_spec = default_layout);
// // context-based interface
// template<class Context>
// void
// do_spacer(Context& ctx, layout const& layout_spec = default_layout)
// {
//     do_spacer(get_layout_traversal(ctx), get_data_traversal(ctx),
//     layout_spec);
// }

// // This version of the spacer records the region that's assigned to it.
// void
// do_spacer(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout_box* region,
//     layout const& layout_spec = default_layout);
// // context-based interface
// template<class Context>
// void
// do_spacer(
//     Context& ctx,
//     layout_box* region,
//     layout const& layout_spec = default_layout)
// {
//     do_spacer(
//         get_layout_traversal(ctx),
//         get_data_traversal(ctx),
//         region,
//         layout_spec);
// }

template<class Logic>
layout_box
get_container_region(simple_layout_container<Logic> const& container)
{
    return layout_box(make_layout_vector(0, 0), container.assigned_size);
}

template<class Logic>
layout_box
get_padded_container_region(simple_layout_container<Logic> const& container)
{
    return layout_box(
        container.cacher.relative_assignment.region.corner
            - container.cacher.resolved_relative_assignment.region.corner,
        container.cacher.relative_assignment.region.size);
}

template<class Logic>
layout_vector
get_container_offset(simple_layout_container<Logic> const& container)
{
    return get_assignment(container.cacher).region.corner;
}

// #define ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(container_name, logic_type)      \
//     struct logic_type;                                                        \
//     struct container_name                                                     \
//     {                                                                         \
//         container_name() : container_(0)                                      \
//         {                                                                     \
//         }                                                                     \
//                                                                               \
//         template<class Context>                                               \
//         container_name(                                                       \
//             Context& ctx, layout const& layout_spec = default_layout)         \
//         {                                                                     \
//             begin(ctx, layout_spec);                                          \
//         }                                                                     \
//                                                                               \
//         ~container_name()                                                     \
//         {                                                                     \
//             end();                                                            \
//         }                                                                     \
//                                                                               \
//         template<class Context>                                               \
//         void                                                                  \
//         begin(Context& ctx, layout const& layout_spec = default_layout)       \
//         {                                                                     \
//             concrete_begin(                                                   \
//                 get_layout_traversal(ctx),                                    \
//                 get_data_traversal(ctx),                                      \
//                 layout_spec);                                                 \
//         }                                                                     \
//                                                                               \
//         void                                                                  \
//         end()                                                                 \
//         {                                                                     \
//             if (container_)                                                   \
//             {                                                                 \
//                 slc_.end();                                                   \
//                 container_ = 0;                                               \
//             }                                                                 \
//         }                                                                     \
//                                                                               \
//         layout_box                                                            \
//         region() const                                                        \
//         {                                                                     \
//             return get_container_region(*container_);                         \
//         }                                                                     \
//                                                                               \
//         layout_box                                                            \
//         padded_region() const                                                 \
//         {                                                                     \
//             return get_padded_container_region(*container_);                  \
//         }                                                                     \
//                                                                               \
//         layout_vector                                                         \
//         offset() const                                                        \
//         {                                                                     \
//             return get_container_offset(*container_);                         \
//         }                                                                     \
//                                                                               \
//      private:                                                                 \
//         void                                                                  \
//         concrete_begin(                                                       \
//             layout_traversal& traversal,                                      \
//             data_traversal& data,                                             \
//             layout const& layout_spec);                                       \
//                                                                               \
//         simple_layout_container<logic_type>* container_;                      \
//         scoped_layout_container slc_;                                         \
//     };

// #define ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(                        \
//     container_name, logic_type, Arg)                                          \
//     struct logic_type;                                                        \
//     struct container_name                                                     \
//     {                                                                         \
//         container_name()                                                      \
//         {                                                                     \
//         }                                                                     \
//                                                                               \
//         template<class Context>                                               \
//         container_name(                                                       \
//             Context& ctx,                                                     \
//             Arg arg,                                                          \
//             layout const& layout_spec = default_layout)                       \
//         {                                                                     \
//             begin(ctx, arg, layout_spec);                                     \
//         }                                                                     \
//                                                                               \
//         ~container_name()                                                     \
//         {                                                                     \
//             end();                                                            \
//         }                                                                     \
//                                                                               \
//         template<class Context>                                               \
//         void                                                                  \
//         begin(                                                                \
//             Context& ctx,                                                     \
//             Arg arg,                                                          \
//             layout const& layout_spec = default_layout)                       \
//         {                                                                     \
//             concrete_begin(                                                   \
//                 get_layout_traversal(ctx),                                    \
//                 get_data_traversal(ctx),                                      \
//                 arg,                                                          \
//                 layout_spec);                                                 \
//         }                                                                     \
//                                                                               \
//         void                                                                  \
//         end()                                                                 \
//         {                                                                     \
//             slc_.end();                                                       \
//         }                                                                     \
//                                                                               \
//         layout_box                                                            \
//         region() const                                                        \
//         {                                                                     \
//             return get_container_region(*container_);                         \
//         }                                                                     \
//                                                                               \
//         layout_box                                                            \
//         padded_region() const                                                 \
//         {                                                                     \
//             return get_padded_container_region(*container_);                  \
//         }                                                                     \
//                                                                               \
//         layout_vector                                                         \
//         offset() const                                                        \
//         {                                                                     \
//             return get_container_offset(*container_);                         \
//         }                                                                     \
//                                                                               \
//      private:                                                                 \
//         void                                                                  \
//         concrete_begin(                                                       \
//             layout_traversal& traversal,                                      \
//             data_traversal& data,                                             \
//             Arg arg,                                                          \
//             layout const& layout_spec);                                       \
//                                                                               \
//         simple_layout_container<logic_type>* container_;                      \
//         scoped_layout_container slc_;                                         \
//     };

// // A row layout places all its children in a horizontal row.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(row_layout)

// // A column layout places all its children in a vertical column.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(column_layout)

// // Linear layout places its children in a line.
// // In other words, it can act as either a row or a column depending on the
// // argument you give it.
// ALIA_DEFINE_FLAG_TYPE(linear_layout)
// ALIA_DEFINE_FLAG(linear_layout, 0, HORIZONTAL_LAYOUT)
// ALIA_DEFINE_FLAG(linear_layout, 1, VERTICAL_LAYOUT)
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(
//     linear_layout, linear_layout_flag_set)

// // Layered layout places all its children in the same rectangle, so they are
// // in effect layered over the same region.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(layered_layout)

// // A rotated layout rotates its child 90 degrees counterclockwise.
// // Note that a rotated layout should only have one child. If it has multiple
// // children, it will simply layer them.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(rotated_layout)

// // A flow layout arranges its children in horizontal rows, wrapping them
// around
// // to new rows as needed.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(flow_layout)

// // A vertical flow layout arranges its children in columns. Widgets flow
// down
// // the columns, starting with the left column. Note that like all
// containers,
// // this one is still primarily driven by the horizontal space allocated to
// it,
// // so it will prefer to create many short columns rather than one long one.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(vertical_flow_layout)

// // clamped_layout imposes a maximum size on its child.
// // (It should only have a single child.)
// // If the space assigned to it is larger than the specified maximum, it
// // centers the child within that space.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(clamped_layout, absolute_size)

// // bordered_layout adds a border to its child.
// // (It should only have a single child.)
// // (Note that the scalar type should ideally be relative_length, but that's
// a
// // little more complicated to implement.)
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(
//     bordered_layout, box_border_width<absolute_length>)

// // A clip_evasion_layout will move its child around inside its assigned
// region
// // to try to keep the child visible.
// ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(clip_evasion_layout)

// #define ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid_type, row_type)               \
//     struct grid_type##_data;                                                  \
//     struct grid_type                                                          \
//     {                                                                         \
//         grid_type()                                                           \
//         {                                                                     \
//         }                                                                     \
//                                                                               \
//         template<class Context>                                               \
//         grid_type(                                                            \
//             Context& ctx,                                                     \
//             layout const& layout_spec = default_layout,                       \
//             absolute_length const& column_spacing                             \
//             = absolute_length(0, PIXELS))                                     \
//         {                                                                     \
//             begin(ctx, layout_spec, column_spacing);                          \
//         }                                                                     \
//                                                                               \
//         ~grid_type()                                                          \
//         {                                                                     \
//             end();                                                            \
//         }                                                                     \
//                                                                               \
//         template<class Context>                                               \
//         void                                                                  \
//         begin(                                                                \
//             Context& ctx,                                                     \
//             layout const& layout_spec = default_layout,                       \
//             absolute_length const& column_spacing                             \
//             = absolute_length(0, PIXELS))                                     \
//         {                                                                     \
//             concrete_begin(                                                   \
//                 get_layout_traversal(ctx),                                    \
//                 get_data_traversal(ctx),                                      \
//                 layout_spec,                                                  \
//                 column_spacing);                                              \
//         }                                                                     \
//                                                                               \
//         void                                                                  \
//         end()                                                                 \
//         {                                                                     \
//             container_.end();                                                 \
//         }                                                                     \
//                                                                               \
//      private:                                                                 \
//         void                                                                  \
//         concrete_begin(                                                       \
//             layout_traversal& traversal,                                      \
//             data_traversal& data,                                             \
//             layout const& layout_spec,                                        \
//             absolute_length const& column_spacing);                           \
//                                                                               \
//         friend struct row_type;                                               \
//                                                                               \
//         scoped_layout_container container_;                                   \
//         layout_traversal* traversal_;                                         \
//         data_traversal* data_traversal_;                                      \
//         grid_type##_data* data_;                                              \
//     };                                                                        \
//     struct row_type                                                           \
//     {                                                                         \
//         row_type()                                                            \
//         {                                                                     \
//         }                                                                     \
//         row_type(                                                             \
//             grid_type const& g, layout const& layout_spec = default_layout)   \
//         {                                                                     \
//             begin(g, layout_spec);                                            \
//         }                                                                     \
//         ~row_type()                                                           \
//         {                                                                     \
//             end();                                                            \
//         }                                                                     \
//         void                                                                  \
//         begin(                                                                \
//             grid_type const& g, layout const& layout_spec = default_layout);  \
//         void                                                                  \
//         end();                                                                \
//                                                                               \
//      private:                                                                 \
//         scoped_layout_container container_;                                   \
//     };

// // A grid layout is used to arrange widgets in a grid. To use it, create
// // grid_row containers that reference the grid_layout.
// // Note that the grid_layout container by itself is just a normal column, so
// // you can intersperse other widgets amongst the grid rows.
// ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid_layout, grid_row)

// // A uniform grid layout is similar to a grid layout but all cells in the
// grid
// // have a uniform size.
// ALIA_DECLARE_GRID_LAYOUT_CONTAINER(uniform_grid_layout, uniform_grid_row)

// // A floating_layout detaches its contents from the parent context.
// // It should only have one child (generally another container).
// // This child becomes the root of an independent layout tree.
// // The caller may specify the minimum and maximum size of the child.
// // Note that the child is simply placed at (0, 0).
// // It's assumed that the caller will take care of positioning it properly.
// struct floating_layout_data;
// struct floating_layout
// {
//     floating_layout() : traversal_(0)
//     {
//     }

//     template<class Context>
//     floating_layout(
//         Context& ctx,
//         layout_vector const& min_size = make_layout_vector(-1, -1),
//         layout_vector const& max_size = make_layout_vector(-1, -1))
//     {
//         begin(ctx, min_size, max_size);
//     }

//     ~floating_layout()
//     {
//         end();
//     }

//     template<class Context>
//     void
//     begin(
//         Context& ctx,
//         layout_vector const& min_size = make_layout_vector(-1, -1),
//         layout_vector const& max_size = make_layout_vector(-1, -1))
//     {
//         concrete_begin(
//             get_layout_traversal(ctx),
//             get_data_traversal(ctx),
//             min_size,
//             max_size);
//     }

//     void
//     end();

//     layout_vector
//     size() const;

//     layout_box
//     region() const
//     {
//         return layout_box(make_layout_vector(0, 0), size());
//     }

//  private:
//     void
//     concrete_begin(
//         layout_traversal& traversal,
//         data_traversal& data,
//         layout_vector const& min_size,
//         layout_vector const& max_size);

//     layout_traversal* traversal_;
//     widget_container* old_container_;
//     layout_node** old_next_ptr_;
//     layout_node* floating_root_;
//     floating_layout_data* data_;
//     // scoped_clip_region_reset clipping_reset_;
//     layout_vector min_size_, max_size_;
// };

} // namespace indie
} // namespace alia

#endif
