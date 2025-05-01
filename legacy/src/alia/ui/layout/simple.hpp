#ifndef ALIA_UI_LAYOUT_SIMPLE_HPP
#define ALIA_UI_LAYOUT_SIMPLE_HPP

#include <alia/ui/layout/geometry.hpp>
#include <alia/ui/layout/specification.hpp>
// TODO: This shouldn't be here.
#include <alia/ui/layout/utilities.hpp>

namespace alia {

struct data_traversal;
struct layout_traversal;
struct layout_system;
struct layout_container;
struct layout_node;
struct layout_style_info;
template<class Logic>
struct simple_layout_container;

#define ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(container_name, logic_type)      \
    struct logic_type;                                                        \
    struct container_name                                                     \
    {                                                                         \
        container_name() : container_(0)                                      \
        {                                                                     \
        }                                                                     \
                                                                              \
        template<class Context>                                               \
        container_name(                                                       \
            Context& ctx, layout const& layout_spec = default_layout)         \
        {                                                                     \
            begin(ctx, layout_spec);                                          \
        }                                                                     \
        template<class Context>                                               \
        container_name(                                                       \
            Context& ctx,                                                     \
            simple_layout_container<logic_type>& container,                   \
            layout const& layout_spec = default_layout)                       \
        {                                                                     \
            begin(ctx, container, layout_spec);                               \
        }                                                                     \
                                                                              \
        ~container_name()                                                     \
        {                                                                     \
            end();                                                            \
        }                                                                     \
                                                                              \
        template<class Context>                                               \
        void                                                                  \
        begin(Context& ctx, layout const& layout_spec = default_layout)       \
        {                                                                     \
            concrete_begin(                                                   \
                get_layout_traversal(ctx),                                    \
                get_data_traversal(ctx),                                      \
                nullptr,                                                      \
                layout_spec);                                                 \
        }                                                                     \
                                                                              \
        template<class Context>                                               \
        void                                                                  \
        begin(                                                                \
            Context& ctx,                                                     \
            simple_layout_container<logic_type>& container,                   \
            layout const& layout_spec = default_layout)                       \
        {                                                                     \
            concrete_begin(                                                   \
                get_layout_traversal(ctx),                                    \
                get_data_traversal(ctx),                                      \
                &container,                                                   \
                layout_spec);                                                 \
        }                                                                     \
                                                                              \
        void                                                                  \
        end()                                                                 \
        {                                                                     \
            if (container_)                                                   \
            {                                                                 \
                transform_.end();                                             \
                slc_.end();                                                   \
                container_ = 0;                                               \
            }                                                                 \
        }                                                                     \
                                                                              \
        layout_box                                                            \
        region() const                                                        \
        {                                                                     \
            return get_container_region(*container_);                         \
        }                                                                     \
                                                                              \
        layout_box                                                            \
        padded_region() const                                                 \
        {                                                                     \
            return get_padded_container_region(*container_);                  \
        }                                                                     \
                                                                              \
        layout_vector                                                         \
        offset() const                                                        \
        {                                                                     \
            return get_container_offset(*container_);                         \
        }                                                                     \
                                                                              \
     private:                                                                 \
        void                                                                  \
        concrete_begin(                                                       \
            layout_traversal& traversal,                                      \
            data_traversal& data,                                             \
            simple_layout_container<logic_type>* container,                   \
            layout const& layout_spec);                                       \
                                                                              \
        simple_layout_container<logic_type>* container_;                      \
        scoped_layout_container slc_;                                         \
        scoped_transformation transform_;                                     \
    };                                                                        \
    using container_name##_container = simple_layout_container<logic_type>;

#define ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(                        \
    container_name, logic_type, Arg)                                          \
    struct container_name                                                     \
    {                                                                         \
        container_name()                                                      \
        {                                                                     \
        }                                                                     \
                                                                              \
        template<class Context>                                               \
        container_name(                                                       \
            Context& ctx,                                                     \
            Arg arg,                                                          \
            layout const& layout_spec = default_layout)                       \
        {                                                                     \
            begin(ctx, arg, layout_spec);                                     \
        }                                                                     \
                                                                              \
        ~container_name()                                                     \
        {                                                                     \
            end();                                                            \
        }                                                                     \
                                                                              \
        template<class Context>                                               \
        void                                                                  \
        begin(                                                                \
            Context& ctx,                                                     \
            Arg arg,                                                          \
            layout const& layout_spec = default_layout)                       \
        {                                                                     \
            concrete_begin(                                                   \
                get_layout_traversal(ctx),                                    \
                get_data_traversal(ctx),                                      \
                arg,                                                          \
                layout_spec);                                                 \
        }                                                                     \
                                                                              \
        void                                                                  \
        end()                                                                 \
        {                                                                     \
            transform_.end();                                                 \
            slc_.end();                                                       \
        }                                                                     \
                                                                              \
        layout_box                                                            \
        region() const                                                        \
        {                                                                     \
            return get_container_region(*container_);                         \
        }                                                                     \
                                                                              \
        layout_box                                                            \
        padded_region() const                                                 \
        {                                                                     \
            return get_padded_container_region(*container_);                  \
        }                                                                     \
                                                                              \
        layout_vector                                                         \
        offset() const                                                        \
        {                                                                     \
            return get_container_offset(*container_);                         \
        }                                                                     \
                                                                              \
     private:                                                                 \
        void                                                                  \
        concrete_begin(                                                       \
            layout_traversal& traversal,                                      \
            data_traversal& data,                                             \
            Arg arg,                                                          \
            layout const& layout_spec);                                       \
                                                                              \
        simple_layout_container<logic_type>* container_;                      \
        scoped_layout_container slc_;                                         \
        scoped_transformation transform_;                                     \
    };                                                                        \
    using container_name##_container = simple_layout_container<logic_type>;

// A row layout places all its children in a horizontal row.
ALIA_DECLARE_LAYOUT_LOGIC(row_layout_logic)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(row_layout, row_layout_logic)

// A column layout places all its children in a vertical column.
ALIA_DECLARE_LAYOUT_LOGIC(column_layout_logic)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(column_layout, column_layout_logic)

// Layered layout places all its children in the same rectangle, so they are in
// effect layered over the same region.
ALIA_DECLARE_LAYOUT_LOGIC(layered_layout_logic)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(layered_layout, layered_layout_logic)

// A rotated layout rotates its child 90 degrees counterclockwise.
// Note that a rotated layout should only have one child. If it has multiple
// children, it will simply layer them.
ALIA_DECLARE_LAYOUT_LOGIC(rotated_layout_logic)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(rotated_layout, rotated_layout_logic)

// A flow layout arranges its children in horizontal rows, wrapping them around
// to new rows as needed.
ALIA_DECLARE_LAYOUT_LOGIC_WITH_DATA(flow_layout_logic,
                                    layout_flag_set x_alignment_;)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(flow_layout, flow_layout_logic)

// A vertical flow layout arranges its children in columns. Widgets flow down
// the columns, starting with the left column. Note that like all containers,
// this one is still primarily driven by the horizontal space allocated to it,
// so it will prefer to create many short columns rather than one long one.
ALIA_DECLARE_LAYOUT_LOGIC(vertical_flow_layout_logic)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(
    vertical_flow_layout, vertical_flow_layout_logic)

// clamped_layout imposes a maximum size on its child.
// (It should only have a single child.)
// If the space assigned to it is larger than the specified maximum, it
// centers the child within that space.
ALIA_DECLARE_LAYOUT_LOGIC_WITH_DATA(clamped_layout_logic,
                                    layout_vector max_size;)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(
    clamped_layout, clamped_layout_logic, absolute_size)

// bordered_layout adds a border to its child.
// (It should only have a single child.)
// (Note that the scalar type should ideally be relative_length, but that's a
// little more complicated to implement.)
ALIA_DECLARE_LAYOUT_LOGIC_WITH_DATA(bordered_layout_logic,
                                    box_border_width<layout_scalar> border;)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER_WITH_ARG(
    bordered_layout, bordered_layout_logic, box_border_width<absolute_length>)

// A clip_evasion_layout will move its child around inside its assigned region
// to try to keep the child visible.
ALIA_DECLARE_LAYOUT_LOGIC(clip_evasion_layout_logic)
ALIA_DECLARE_SIMPLE_LAYOUT_CONTAINER(
    clip_evasion_layout, clip_evasion_layout_logic)

} // namespace alia

#endif
