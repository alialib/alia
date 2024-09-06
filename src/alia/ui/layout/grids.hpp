#ifndef ALIA_UI_LAYOUT_GRIDS_HPP
#define ALIA_UI_LAYOUT_GRIDS_HPP

#include <alia/ui/layout/geometry.hpp>
#include <alia/ui/layout/specification.hpp>
// TODO: This shouldn't be here.
#include <alia/ui/layout/utilities.hpp>

namespace alia {

struct data_traversal;
struct layout_traversal;
struct layout_container;
struct layout_node;

#define ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid_type, row_type)               \
    struct grid_type##_data;                                                  \
    struct grid_type                                                          \
    {                                                                         \
        grid_type()                                                           \
        {                                                                     \
        }                                                                     \
                                                                              \
        template<class Context>                                               \
        grid_type(                                                            \
            Context& ctx,                                                     \
            layout const& layout_spec = default_layout,                       \
            absolute_length const& column_spacing                             \
            = absolute_length(0, PIXELS))                                     \
        {                                                                     \
            begin(ctx, layout_spec, column_spacing);                          \
        }                                                                     \
                                                                              \
        ~grid_type()                                                          \
        {                                                                     \
            end();                                                            \
        }                                                                     \
                                                                              \
        template<class Context>                                               \
        void                                                                  \
        begin(                                                                \
            Context& ctx,                                                     \
            layout const& layout_spec = default_layout,                       \
            absolute_length const& column_spacing                             \
            = absolute_length(0, PIXELS))                                     \
        {                                                                     \
            concrete_begin(                                                   \
                get_layout_traversal(ctx),                                    \
                get_data_traversal(ctx),                                      \
                layout_spec,                                                  \
                column_spacing);                                              \
        }                                                                     \
                                                                              \
        void                                                                  \
        end()                                                                 \
        {                                                                     \
            transform_.end();                                                 \
            container_.end();                                                 \
        }                                                                     \
                                                                              \
     private:                                                                 \
        void                                                                  \
        concrete_begin(                                                       \
            layout_traversal& traversal,                                      \
            data_traversal& data,                                             \
            layout const& layout_spec,                                        \
            absolute_length const& column_spacing);                           \
                                                                              \
        friend struct row_type;                                               \
                                                                              \
        scoped_layout_container container_;                                   \
        scoped_transformation transform_;                                     \
        layout_traversal* traversal_;                                         \
        data_traversal* data_traversal_;                                      \
        grid_type##_data* data_;                                              \
    };                                                                        \
    struct row_type                                                           \
    {                                                                         \
        row_type()                                                            \
        {                                                                     \
        }                                                                     \
        row_type(                                                             \
            grid_type const& g, layout const& layout_spec = default_layout)   \
        {                                                                     \
            begin(g, layout_spec);                                            \
        }                                                                     \
        ~row_type()                                                           \
        {                                                                     \
            end();                                                            \
        }                                                                     \
        void                                                                  \
        begin(                                                                \
            grid_type const& g, layout const& layout_spec = default_layout);  \
        void                                                                  \
        end();                                                                \
                                                                              \
     private:                                                                 \
        scoped_layout_container container_;                                   \
        scoped_transformation transform_;                                     \
    };

// A grid layout is used to arrange widgets in a grid. To use it, create
// grid_row containers that reference the grid_layout.
// Note that the grid_layout container by itself is just a normal column, so
// you can intersperse other widgets amongst the grid rows.
ALIA_DECLARE_GRID_LAYOUT_CONTAINER(grid_layout, grid_row)

// A uniform grid layout is similar to a grid layout but all cells in the grid
// have a uniform size.
ALIA_DECLARE_GRID_LAYOUT_CONTAINER(uniform_grid_layout, uniform_grid_row)

} // namespace alia

#endif
