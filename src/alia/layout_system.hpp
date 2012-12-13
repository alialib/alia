#ifndef ALIA_LAYOUT_SYSTEM_HPP
#define ALIA_LAYOUT_SYSTEM_HPP

#include <alia/layout_interface.hpp>
#include <alia/data_graph.hpp>
#include <vector>

// This file defines the layout system for alia, along with as some utilities
// for facilitating the common use cases of the system.

// alia's layout system uses an immediate mode interface for specifying the
// contents of the UI. However, such an interface imposes limitations on the
// types of layout algorithms that can be implemented efficiently.
// Thus, alia builds an internal, hierarchical representation of the UI tree
// that is specified by the application, and evaluates its layout by directly
// querying the nodes in that tree. It also uses this representation to track
// changes in the tree and detect when it can reuse cached results.

namespace alia {

// A lot of layout calculations are done in floating point and then converted
// to integer coordinates (e.g., by rounding up sizes to a full pixel).
// This should be done via the following functions in case the layout_scalar
// type is changed.

// Cast a floating point number to a layout scalar representing a size.
static inline layout_scalar as_layout_size(double x)
{ return layout_scalar(std::ceil(x)); }
static inline layout_scalar as_layout_size(float x)
{ return layout_scalar(std::ceil(x)); }
static inline layout_scalar as_layout_size(int x)
{ return layout_scalar(x); }
// Cast a floating point number to a layout scalar by rounding.
static inline layout_scalar round_to_layout_scalar(float x)
{ return layout_scalar(x + 0.5); }
static inline layout_scalar round_to_layout_scalar(double x)
{ return layout_scalar(x + 0.5); }

// Test if two layout scalars are almost equal to each other.
static inline bool layout_scalars_almost_equal(
    layout_scalar a, layout_scalar b)
{
    return a == b;
}

layout_scalar const layout_scalar_epsilon = 1;

// forward declarations of the structures that make up the layout tree
struct layout_node;
struct layout_container;

// layout_requirements stores the requirements for a layout element along a
// single axis.
struct layout_requirements
{
    layout_scalar minimum_size;

    // The minimum space required on either side of the baseline.
    layout_scalar minimum_ascent, minimum_descent;

    // If these is extra space in a container (above the minimum required by
    // its contents), it's apportioned to the individual widgets according to
    // the values they specify here.
    float growth_factor;

    layout_requirements() {}

    layout_requirements(layout_scalar minimum_size,
        layout_scalar minimum_ascent, layout_scalar minimum_descent,
        float growth_factor)
      : minimum_size(minimum_size), minimum_ascent(minimum_ascent),
        minimum_descent(minimum_descent), growth_factor(growth_factor)
    {}
};

// Update 'current' so that it includes the additional requirements specified
// by 'additional'.
void fold_in_requirements(layout_requirements& current,
    layout_requirements const& additional);

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
    bool is_padded;
    vector<2,int> padding_size;

    float font_size;
    vector<2,float> character_size;
    float x_height;
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
static inline data_traversal& get_data_traversal(
    layout_calculation_context& ctx)
{ return ctx.data; }
static inline naming_map& get_naming_map(layout_calculation_context& ctx)
{ return *ctx.map; }

// layout_system stores the persistent (interpass) state associated with an
// instance of the layout system.
struct layout_system
{
    counter_type refresh_counter;

    layout_node* root_node;

    data_graph calculation_cache;

    layout_system()
      : refresh_counter(0)
      , root_node(0)
    {}
};

// scoped_layout_traversal sets up a non-refresh traversal for a layout system.
struct scoped_layout_traversal
{
    scoped_layout_traversal() {}

    scoped_layout_traversal(
        layout_system& system, layout_traversal& traversal,
        data_traversal& data, geometry_context& geometry,
        vector<2,float> const& ppi)
    { begin(system, traversal, data, geometry, ppi); }

    ~scoped_layout_traversal() { end(); }

    void begin(layout_system& system, layout_traversal& traversal,
        data_traversal& data, geometry_context& geometry,
        vector<2,float> const& ppi);

    void end() {}

 private:
    layout_style_info dummy_style_info_;
};

// scoped_layout_refresh sets up a refresh for a layout system.
struct scoped_layout_refresh
{
    scoped_layout_refresh() {}

    scoped_layout_refresh(
        layout_system& system, layout_traversal& traversal,
        data_traversal& data, vector<2,float> const& ppi)
    { begin(system, traversal, data, ppi); }

    ~scoped_layout_refresh() { end(); }

    void begin(layout_system& system, layout_traversal& traversal,
        data_traversal& data, vector<2,float> const& ppi);

    void end() {}

 private:
    layout_style_info dummy_style_info_;
};

// scoped_layout_traversal sets up a calculation context for a layout system.
struct scoped_layout_calculation_context
{
    scoped_layout_calculation_context() {}

    scoped_layout_calculation_context(
        data_graph& cache, layout_calculation_context& ctx)
    { begin(cache, ctx); }

    ~scoped_layout_calculation_context() { end(); }

    void begin(data_graph& cache, layout_calculation_context& ctx);

    void end();

private:
    scoped_data_traversal data_;
};

// Given the available space, calculate the proper regions for all nodes in the
// given layout system.
void resolve_layout(layout_system& system, layout_vector const& size);
// Same, but with arguments broken up for flexibility.
void resolve_layout(layout_node* root_node, data_graph& cache,
    layout_vector const& size);

// Calculate the minimum space needed by the given layout system.
layout_vector get_minimum_size(layout_system& system);
// Same, but with arguments broken up for flexibility.
layout_vector get_minimum_size(layout_node* root_node, data_graph& cache);

layout add_default_size(layout const& layout_spec, size const& size);
layout add_default_padding(layout const& layout_spec, layout_flag_set flag);
layout add_default_x_alignment(layout const& layout_spec,
    layout_flag_set alignment);
layout add_default_y_alignment(layout const& layout_spec,
    layout_flag_set alignment);
layout add_default_alignment(layout const& layout_spec,
    layout_flag_set x_alignment, layout_flag_set y_alignment);

struct wrapped_row
{
    layout_requirements requirements;
    layout_scalar y;
};

struct wrapping_state
{
    std::vector<wrapped_row>* rows;
    wrapped_row active_row;
    layout_scalar accumulated_width;
};
void wrap_row(wrapping_state& state);

struct wrapping_assignment_state
{
    std::vector<wrapped_row>::iterator active_row;
    layout_scalar x;
};
void wrap_row(wrapping_assignment_state& state);

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
        layout_scalar assigned_width,
        wrapping_state& state);
    virtual void assign_wrapped_regions(
        layout_calculation_context& ctx,
        layout_scalar assigned_width,
        wrapping_assignment_state& state);

    // next node in the list of siblings
    layout_node* next;
};

// Alternate forms for invoking the layout_node interface.
static inline layout_requirements get_horizontal_requirements(
    layout_calculation_context& ctx, layout_node& node)
{
    return node.get_horizontal_requirements(ctx);
}
static inline layout_requirements get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node& node,
    layout_scalar assigned_width)
{
    return node.get_vertical_requirements(ctx, assigned_width);
}
static inline void set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node& node,
    relative_layout_assignment const& assignment)
{
    node.set_relative_assignment(ctx, assignment);
}

// All nodes in a layout tree with children derive from this.
struct layout_container : layout_node
{
    layout_node* children;

    // This records the last refresh in which the contents of the container
    // changed. It's updated during the refresh pass and is used to determine
    // when the container's layout needs to be recomputed.
    counter_type last_content_change;

    layout_container* parent;

    layout_container() : children(0), last_content_change(0), parent(0) {}
};
// Initialize a container for use within the given context.
void initialize(layout_traversal& traversal, layout_container& container);

// Add a layout node to the layout tree being traversed.
void add_layout_node(layout_traversal& traversal, layout_node* node);

// Record a change in the layout at the current position in the traversal.
void record_layout_change(layout_traversal& traversal);

// detect_layout_change(ctx, value_storage, new_value) detects if new_value is
// different from the value stored in value_storage.
// If it is, it records the change in the layout tree and updates the stored
// value.
// The return value indicates whether or not a change was detected.
template<class Context, class T>
bool detect_layout_change(Context& ctx, T* value_storage, T const& new_value)
{
    layout_traversal& traversal = get_layout_traversal(ctx);
    if (traversal.is_refresh_pass && *value_storage != new_value)
    {
        record_layout_change(traversal);
        *value_storage = new_value;
        return true;
    }
    return false;
}

// Resolve a user size specification into an actual size, in pixels.
// The result may depend on the state of the traversal (e.g., the current
// font size).
layout_scalar resolve_layout_width(layout_traversal& traversal, float width,
    layout_units units);
layout_scalar resolve_layout_height(layout_traversal& traversal, float height,
    layout_units units);
layout_vector resolve_layout_size(layout_traversal& traversal, size const& s);

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

    resolved_layout_spec() {}
    resolved_layout_spec(
        layout_vector const& size,
        layout_flag_set flags,
        float growth_factor,
        layout_vector const& padding_size)
      : size(size), flags(flags), growth_factor(growth_factor),
        padding_size(padding_size)
    {}
};
bool operator==(resolved_layout_spec const& a, resolved_layout_spec const& b);
bool operator!=(resolved_layout_spec const& a, resolved_layout_spec const& b);
void resolve_layout_spec(
    layout_traversal& traversal, resolved_layout_spec& resolved,
    layout const& spec, layout_flag_set default_flags);

// The layout of a UI element is also dependent on the calculated minimum
// requirements for that element.
struct calculated_layout_requirements
{
    layout_scalar minimum_size;

    // The minimum space required on either side of the baseline.
    layout_scalar minimum_ascent, minimum_descent;

    calculated_layout_requirements() {}

    calculated_layout_requirements(layout_scalar minimum_size,
        layout_scalar minimum_ascent, layout_scalar minimum_descent)
      : minimum_size(minimum_size), minimum_ascent(minimum_ascent),
        minimum_descent(minimum_descent)
    {}
};

// Given a resolved layout spec from the application, and the calculated
// requirements of the UI element, this fills in the actual requirements for
// the UI element along the selected axis.
void resolve_requirements(
    layout_requirements& requirements, resolved_layout_spec const& spec,
    unsigned axis, calculated_layout_requirements const& calculated);

// A UI element may be assigned more width than it needs, so this will
// determine the amount that it should actually use, based on its spec and
// requirements.
layout_scalar resolve_assigned_width(
    resolved_layout_spec const& spec, layout_scalar assigned_width,
    layout_requirements const& horizontal_requirements);

// Again, a UI element may be assigned a bigger region than it requires, so
// this will determine what portion of the region the element should use,
// based on its spec and requirements.
void resolve_relative_assignment(
    relative_layout_assignment& resolved_assignment,
    resolved_layout_spec const& spec,
    relative_layout_assignment const& assignment,
    layout_requirements const& horizontal_requirements,
    layout_requirements const& vertical_requirements);

// layout_cacher is a utility used by layout containers to cache the results
// of their layout calculations.
struct layout_cacher
{
    layout_cacher()
      : last_horizontal_query(0)
      , last_relative_assignment(0)
    {}

    // the resolved layout spec supplied by the user
    resolved_layout_spec resolved_spec;

    // the last frame in which there was a horizontal requirements query
    counter_type last_horizontal_query;
    // the result of that query
    layout_requirements horizontal_requirements;

    // last time set_relative_assignment was called
    counter_type last_relative_assignment;
    // the last value that was passed to set_relative_assignment
    relative_layout_assignment relative_assignment;
    // the actual assignment that that value resolved to
    relative_layout_assignment resolved_relative_assignment;
};
bool update(layout_traversal& traversal, layout_cacher& cacher,
    layout const& layout_spec, layout_flag_set default_flags);
struct horizontal_layout_query
{
    horizontal_layout_query(
        layout_calculation_context& ctx, layout_cacher& cacher,
        counter_type last_content_change);
    bool update_required() const
    { return cacher_->last_horizontal_query != last_content_change_; }
    void update(calculated_layout_requirements const& calculated);
    layout_requirements const& result() const
    { return cacher_->horizontal_requirements; }
 private:
    layout_cacher* cacher_;
    counter_type last_content_change_;
    named_block named_block_;
};
struct cached_vertical_requirements
{
    counter_type last_vertical_query;
    layout_requirements vertical_requirements;
    cached_vertical_requirements() : last_vertical_query(0) {}
};
struct vertical_layout_query
{
    vertical_layout_query(
        layout_calculation_context& ctx,
        layout_cacher& cacher,
        counter_type last_content_change,
        layout_scalar assigned_width);
    bool update_required() const
    { return update_required_; }
    void update(calculated_layout_requirements const& calculated);
    layout_requirements const& result() const
    { return data_->vertical_requirements; }
 private:
    layout_cacher* cacher_;
    cached_vertical_requirements* data_;
    bool update_required_;
    counter_type last_content_change_;
    named_block named_block_;
};
struct relative_region_assignment
{
    relative_region_assignment(
        layout_calculation_context& ctx,
        layout_node& node,
        layout_cacher& cacher,
        counter_type last_content_change,
        relative_layout_assignment const& assignment);
    bool update_required() const
    { return update_required_; }
    relative_layout_assignment const& resolved_assignment() const
    { return cacher_->resolved_relative_assignment; }
    void update();
 private:
    layout_cacher* cacher_;
    counter_type last_content_change_;
    bool update_required_;
    named_block named_block_;
};

// The vast majority of layout containers behave identically except for the
// logic they use to calculate their requirements and divide their space
// amongst their children.
// All the shared behavior is refactored into simple_layout_container.
// layout_logic is implemented for each individual container to define the
// container-specific layout behavior.
struct layout_logic
{
    virtual calculated_layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx,
        layout_node* children) = 0;
    virtual calculated_layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_node* children,
        layout_scalar assigned_width) = 0;
    virtual void set_relative_assignment(
        layout_calculation_context& ctx,
        layout_node* children,
        layout_vector const& size,
        layout_scalar baseline_y) = 0;
};
struct simple_layout_container : layout_container
{
    // implementation of layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    layout_logic* logic;

    layout_cacher cacher;

    layout_vector assigned_size;
};

// get_simple_layout_container is a utility function for retrieving a
// simple_layout_container with a specific type of logic from a UI context's
// data graph and refreshing it.
// (You can optionally provide the storage for it, so the getting part is
// optional.)
template<class Logic>
struct simple_layout_container_storage
{
    simple_layout_container container;
    Logic logic;
};
template<class Context, class Logic>
void get_simple_layout_container(
    Context& ctx,
    simple_layout_container** container,
    Logic** logic,
    layout const& layout_spec,
    simple_layout_container_storage<Logic>* storage = 0)
{
    if (!storage)
        get_data(*get_layout_traversal(ctx).data, &storage);
    storage->container.logic = &storage->logic;

    *container = &storage->container;

    if (is_refresh_pass(ctx))
    {
        if (update(get_layout_traversal(ctx), (*container)->cacher,
                layout_spec, FILL | UNPADDED))
        {
            // Since this container isn't active yet, it didn't get marked as
            // needing recalculation, so we need to do that manually here.
            (*container)->last_content_change =
                get_layout_traversal(ctx).refresh_counter;
        }
    }

    *logic = &storage->logic;
}

// layout_leaf is used to represent simple leaves in the layout tree.
// Each refresh pass, the associated widget function calls refresh_layout(...)
// to pass along the user's layout_spec, the calculated properties of the
// widget, and the default layout flags.
// Then, during subsequent passes, the widget function can retrieve its
// layout assignment by calling assignment().
struct leaf_layout_requirements
{
    leaf_layout_requirements() {}
    leaf_layout_requirements(layout_vector const& size,
        layout_scalar ascent = 0, layout_scalar descent = 0)
      : size(size), ascent(ascent), descent(descent)
    {}
    layout_vector size;
    layout_scalar ascent, descent;
};
struct layout_leaf : layout_node
{
    layout_leaf() /*: initialized_(false)*/ {}

    void refresh_layout(
        layout_traversal& traversal, layout const& layout_spec,
        leaf_layout_requirements const& requirements,
        layout_flag_set default_flags = TOP | LEFT | PADDED);

    relative_layout_assignment const& assignment() const
    { return relative_assignment_; }

    // implementation of layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

 private:
    //bool initialized_;

    // the layout spec supplied by the user
    //layout layout_spec_;

    // the resolved spec
    resolved_layout_spec resolved_spec_;

    // requirements supplied by the widget
    leaf_layout_requirements requirements_;

    // resolved relative assignment
    alia::relative_layout_assignment relative_assignment_;
};

}

#endif
