#ifndef ALIA_LAYOUT_INTERNALS_HPP
#define ALIA_LAYOUT_INTERNALS_HPP

#include <alia/layout/api.hpp>
#include <alia/data_graph.hpp>
#include <vector>

// This file declares various internal types that are necessary to implement
// the layout system and utilities for working with it.

namespace alia {

// layout_requirements stores the requirements for a layout element along a
// single axis.
struct layout_requirements
{
    layout_scalar size;

    // The minimum space required on either side of the baseline.
    layout_scalar ascent, descent;

    // If these is extra space in a container (above the minimum required by
    // its contents), it's apportioned to the individual widgets according to
    // the values they specify here.
    float growth_factor;

    layout_requirements() {}

    layout_requirements(layout_scalar size, layout_scalar ascent,
        layout_scalar descent, float growth_factor)
      : size(size), ascent(ascent), descent(descent),
        growth_factor(growth_factor)
    {}
};

// relative_layout_assignment stores the information that's passed down to a
// node when it's assigned a relative position and size within its parent.
struct relative_layout_assignment
{
    layout_box region;

    // the distance from the top of the region to the baseline
    layout_scalar baseline_y;

    relative_layout_assignment() {}

    relative_layout_assignment(
        layout_box const& region, layout_scalar baseline_y)
      : region(region), baseline_y(baseline_y)
    {}
};
static inline bool operator==(relative_layout_assignment const& a,
    relative_layout_assignment const& b)
{
    return a.region == b.region && a.baseline_y == b.baseline_y;
}
static inline bool operator!=(relative_layout_assignment const& a,
    relative_layout_assignment const& b)
{
    return !(a == b);
}

// This information is required by the layout system to resolve layout
// specifications that refer to the current style.
struct layout_style_info
{
    vector<2,int> padding_size;
    float font_size;
    vector<2,float> character_size;
    float x_height;
    float magnification;
};

// As in the data_graph library, the layout system may be used in a larger
// context, so utilities that want to operate directly on that context will
// accept any context parameter and call get_layout_traversal(ctx).

static inline layout_traversal& get_layout_traversal(layout_traversal& ctx)
{ return ctx; }

// layout_calculation_context is a context for calculating layout.
// It contains a data traversal for caching layout results.
struct layout_calculation_context
{
    data_traversal data;
    naming_map* map;
    bool for_measurement;
};
static inline data_traversal&
get_data_traversal(layout_calculation_context& ctx)
{ return ctx.data; }
static inline naming_map&
get_naming_map(layout_calculation_context& ctx)
{ return *ctx.map; }

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
    virtual ~layout_node() {}

    // the interface required to be implemented by all layout nodes
    virtual layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx) = 0;
    virtual layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width) = 0;
    virtual void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment) = 0;

    // an alternate interface for nodes that can wrap
    // (Default implementations are supplied for nodes that don't wrap.)
    virtual layout_requirements get_minimal_horizontal_requirements(
        layout_calculation_context& ctx);
    virtual void calculate_wrapping(
        layout_calculation_context& ctx,
        wrapping_state& state);
    virtual void assign_wrapped_regions(
        layout_calculation_context& ctx,
        wrapping_assignment_state& state);

    // next node in the list of siblings
    layout_node* next;
};

// All nodes in a layout tree with children derive from this.
struct layout_container : layout_node
{
    layout_node* children;

    // This records the last refresh in which the contents of the container
    // changed. It's updated during the refresh pass and is used to determine
    // when the container's layout needs to be recomputed.
    counter_type last_content_change;

    virtual void record_change(layout_traversal& traversal);

    layout_container* parent;

    layout_container() : children(0), last_content_change(0), parent(0) {}
};

// layout_system stores the persistent (interpass) state associated with an
// instance of the layout system.
struct layout_system
{
    counter_type refresh_counter;

    layout_node* root_node;

    // This is used to cache results of the layout calculation.
    // The entire layout calculation can be viewed as a tree of pure function
    // evaluations.
    // Branches of that tree may be referenced by multiple parents.
    // Also, branches may remain constant across frames.
    // In both cases, it is wasteful to recompute the functions.
    // (In fact, in the former case, it changes the whole complexity of the
    // problem.)
    data_graph calculation_cache;

    // This is used to generate unique IDs for cached layout data.
    counter_type cacher_id_counter;

    layout_system()
      : refresh_counter(1)
      , root_node(0)
      , cacher_id_counter(1)
    {}
};

}

#endif
