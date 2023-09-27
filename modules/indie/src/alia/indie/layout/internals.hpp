#ifndef ALIA_INDIE_LAYOUT_INTERNALS_HPP
#define ALIA_INDIE_LAYOUT_INTERNALS_HPP

#include <vector>

#include <alia/core/flow/data_graph.hpp>

#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/traversal.hpp>

// This file declares various internal types that are necessary to implement
// the layout system and utilities for working with it.

namespace alia { namespace indie {

// layout_requirements stores the requirements for a layout element along a
// single axis.
struct layout_requirements
{
    layout_scalar size = 0;

    // The minimum space required on either side of the baseline.
    layout_scalar ascent = 0, descent = 0;

    // If these is extra space in a container (above the minimum required by
    // its contents), it's apportioned to the individual widgets according to
    // the values they specify here.
    float growth_factor = 0;
};

// relative_layout_assignment stores the information that's passed down to a
// node when it's assigned a relative position and size within its parent.
struct relative_layout_assignment
{
    layout_box region;

    // the distance from the top of the region to the baseline
    layout_scalar baseline_y;
};
inline bool
operator==(
    relative_layout_assignment const& a, relative_layout_assignment const& b)
{
    return a.region == b.region && a.baseline_y == b.baseline_y;
}
inline bool
operator!=(
    relative_layout_assignment const& a, relative_layout_assignment const& b)
{
    return !(a == b);
}

// This information is required by the layout system to resolve layout
// specifications that refer to the current style.
struct layout_style_info
{
    vector<2, layout_scalar> padding_size;
    float font_size;
    vector<2, layout_scalar> character_size;
    float x_height;
    float magnification;
};

// As in the data_graph library, the layout system may be used in a larger
// context, so utilities that want to operate directly on that context will
// accept any context parameter and call get_layout_traversal(ctx).

inline layout_traversal&
get_layout_traversal(layout_traversal& ctx)
{
    return ctx;
}

// These are used when working with wrapped layouts.
struct wrapped_row
{
    layout_scalar width;
    layout_requirements requirements;
    layout_scalar y;
};
struct wrapping_state
{
    std::vector<wrapped_row>* rows;
    layout_scalar assigned_width;
    wrapped_row active_row;
    layout_scalar accumulated_width;
    layout_scalar visible_width;
};
struct wrapping_assignment_state
{
    layout_scalar assigned_width;
    layout_flag_set x_alignment;
    std::vector<wrapped_row>::iterator active_row, end_row;
    layout_scalar x;
};

// layout_nodes make up the layout tree that is built by the system.
struct layout_node
{
    virtual ~layout_node()
    {
    }

    // the interface required to be implemented by all layout nodes
    virtual layout_requirements
    get_horizontal_requirements()
        = 0;
    virtual layout_requirements
    get_vertical_requirements(layout_scalar assigned_width)
        = 0;
    virtual void
    set_relative_assignment(relative_layout_assignment const& assignment)
        = 0;

    // next node in the list of siblings
    layout_node* next = nullptr;
};

// All nodes in a layout tree with children derive from this.
struct layout_container : layout_node
{
    layout_node* children = nullptr;

    layout_container* parent = nullptr;

    // This records the last refresh in which the contents of the container
    // changed. It's updated during the refresh pass and is used to determine
    // when the container's layout needs to be recomputed.
    counter_type last_content_change = 1;

    virtual void
    record_content_change(layout_traversal& traversal);
};

// layout_system stores the persistent state associated with an instance of the
// layout system.
struct layout_system
{
    counter_type refresh_counter = 1;
    layout_node* root_node = nullptr;
};

}} // namespace alia::indie

#endif
