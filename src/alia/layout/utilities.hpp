#ifndef ALIA_LAYOUT_UTILITIES_HPP
#define ALIA_LAYOUT_UTILITIES_HPP

#include <alia/layout/internals.hpp>

// This file defines various utilities for working with the layout system.

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

// Update 'current' so that it includes the additional requirements specified
// by 'additional'.
void fold_in_requirements(layout_requirements& current,
    layout_requirements const& additional);

// Get a unique cacher ID from the layout system.
static inline counter_type get_cacher_id(layout_system& system)
{ return system.cacher_id_counter++; }

// The following are all utility functions for adding default values to
// layout specifications.
layout add_default_size(layout const& layout_spec, size const& size);
layout add_default_padding(layout const& layout_spec, layout_flag_set flag);
layout add_default_x_alignment(layout const& layout_spec,
    layout_flag_set alignment);
layout add_default_y_alignment(layout const& layout_spec,
    layout_flag_set alignment);
layout add_default_alignment(layout const& layout_spec,
    layout_flag_set x_alignment, layout_flag_set y_alignment);

// The following are utilities for working with wrapped layouts.
void wrap_row(wrapping_state& state);
void wrap_row(wrapping_assignment_state& state);

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

// Initialize a container for use within the given context.
void initialize(layout_traversal& traversal, layout_container& container);

void set_next_node(layout_traversal& traversal, layout_node* node);

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
      : id(0)
      , last_horizontal_query(0)
      , last_relative_assignment(0)
    {}

    // the unique ID of this cacher
    counter_type id;

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
bool update_layout_cacher(
    layout_traversal& traversal, layout_cacher& cacher,
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
// Get the resolved relative assignment for a layout cacher.
static inline relative_layout_assignment const&
get_assignment(layout_cacher const& cacher)
{ return cacher.resolved_relative_assignment; }

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
    layout const& layout_spec)
{
    simple_layout_container_storage<Logic>* storage;
    if (get_cached_data(*get_layout_traversal(ctx).data, &storage))
        storage->container.logic = &storage->logic;

    *container = &storage->container;

    if (is_refresh_pass(ctx))
    {
        if (update_layout_cacher(get_layout_traversal(ctx),
                (*container)->cacher, layout_spec, FILL | UNPADDED))
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
    // the resolved spec
    resolved_layout_spec resolved_spec_;

    // requirements supplied by the widget
    leaf_layout_requirements requirements_;

    // resolved relative assignment
    alia::relative_layout_assignment relative_assignment_;
};

// Utilities for working with a geometry context.

void initialize(geometry_context& ctx, box<2,double> const& full_region);

void set_subscriber(geometry_context& ctx,
    geometry_context_subscriber& subscriber);

void set_clip_region(geometry_context& ctx, box<2,double> const& clip_region);

void set_transformation_matrix(geometry_context& ctx,
    matrix<3,3,double> const& matrix);

// Is any part of the given region visible through the clipping rectangle?
bool is_visible(geometry_context& ctx, box<2,double> const& region);

}

#endif
