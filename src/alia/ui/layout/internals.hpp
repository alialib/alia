#ifndef ALIA_UI_LAYOUT_INTERNALS_HPP
#define ALIA_UI_LAYOUT_INTERNALS_HPP

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

// The layout spec supplied by the application must be resolved based on the
// current proporties of the UI context and the default layout properties of
// the UI element that the spec is applied to. This is done by calling
// resolve_layout_spec, which fills out a resolved_layout_spec.
struct resolved_layout_spec
{
    layout_vector size;
    layout_flag_set flags;
    float growth_factor;
    layout_vector padding_size;

    resolved_layout_spec()
    {
    }
    resolved_layout_spec(
        layout_vector const& size,
        layout_flag_set flags,
        float growth_factor,
        layout_vector const& padding_size)
        : size(size),
          flags(flags),
          growth_factor(growth_factor),
          padding_size(padding_size)
    {
    }
};
bool
operator==(resolved_layout_spec const& a, resolved_layout_spec const& b);
bool
operator!=(resolved_layout_spec const& a, resolved_layout_spec const& b);
void
resolve_layout_spec(
    layout_traversal& traversal,
    resolved_layout_spec& resolved,
    layout const& spec,
    layout_flag_set default_flags);

// layout_cacher is a utility used by layout containers to cache the results
// of their layout calculations.
struct layout_cacher
{
    unsigned bits = 0;
    static unsigned constexpr horizontal_query_results_valid = 0;
    static unsigned constexpr vertical_query_results_valid = 1;
    static unsigned constexpr assignment_valid = 2;

    // the resolved layout spec supplied by the user
    resolved_layout_spec resolved_spec;

    // the cached horizontal requirements
    layout_requirements horizontal_requirements;

    // the assigned_width associated with the last vertical query
    layout_scalar assigned_width;
    // the result of that query
    layout_requirements vertical_requirements;

    // the last value that was passed to set_relative_assignment
    relative_layout_assignment relative_assignment;
    // the actual assignment that that value resolved to
    relative_layout_assignment resolved_relative_assignment;
};

// All nodes in a layout tree with children derive from this.
struct layout_container : layout_node
{
    layout_cacher cacher;

    layout_node* children = nullptr;

    layout_container* parent = nullptr;

    virtual void
    record_content_change();
};

// layout_system stores the persistent state associated with an instance of the
// layout system.
struct layout_system
{
    layout_node* root_node = nullptr;
};

} // namespace alia

#endif
