#ifndef ALIA_UI_LAYOUT_NODE_INTERFACE_HPP
#define ALIA_UI_LAYOUT_NODE_INTERFACE_HPP

#include <vector>

#include <alia/core/flow/data_graph.hpp>

#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/traversal.hpp>

// This file declares various internal types that are necessary to implement
// the layout system and utilities for working with it.

namespace alia {

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

struct layout_node_interface
{
    virtual ~layout_node_interface()
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
};

} // namespace alia

#endif
