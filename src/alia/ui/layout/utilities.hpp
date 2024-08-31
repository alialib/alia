#ifndef ALIA_UI_LAYOUT_UTILITIES_HPP
#define ALIA_UI_LAYOUT_UTILITIES_HPP

#include <alia/core/bit_ops.hpp>
#include <alia/ui/layout/internals.hpp>

// This file defines various utilities for working with the layout system.

namespace alia {

// A lot of layout calculations are done in floating point and then converted
// to integer coordinates (e.g., by rounding up sizes to a full pixel).
// This should be done via the following functions in case the layout_scalar
// type is changed.

// Cast a floating point number/vector to a layout scalar representing a size.
inline layout_scalar
as_layout_size(double x)
{
    return layout_scalar(x);
}
inline layout_scalar
as_layout_size(float x)
{
    return layout_scalar(x);
}
inline layout_scalar
as_layout_size(int x)
{
    return layout_scalar(x);
}
inline layout_vector
as_layout_size(vector<2, double> const& x)
{
    return make_vector(as_layout_size(x[0]), as_layout_size(x[1]));
}
inline layout_vector
as_layout_size(vector<2, float> const& x)
{
    return make_vector(as_layout_size(x[0]), as_layout_size(x[1]));
}
inline layout_vector
as_layout_size(vector<2, int> const& x)
{
    return make_vector(as_layout_size(x[0]), as_layout_size(x[1]));
}
// Cast a floating point number to a layout scalar by rounding.
inline layout_scalar
round_to_layout_scalar(float x)
{
    return layout_scalar(x);
}
inline layout_scalar
round_to_layout_scalar(double x)
{
    return layout_scalar(x);
}

// Test if two layout scalars are almost equal to each other.
inline bool
layout_scalars_almost_equal(layout_scalar a, layout_scalar b)
{
    return a == b;
}

layout_scalar const layout_scalar_epsilon = 1;

// Update 'current' so that it includes the additional requirements specified
// by 'additional'.
void
fold_in_requirements(
    layout_requirements& current, layout_requirements const& additional);

// The following are all utility functions for adding default values to
// layout specifications.
layout
add_default_size(layout const& layout_spec, absolute_size const& size);
layout
add_default_padding(layout const& layout_spec, layout_flag_set flag);
layout
add_default_x_alignment(layout const& layout_spec, layout_flag_set alignment);
layout
add_default_y_alignment(layout const& layout_spec, layout_flag_set alignment);
layout
add_default_alignment(
    layout const& layout_spec,
    layout_flag_set x_alignment,
    layout_flag_set y_alignment);

void
set_next_node(layout_traversal& traversal, layout_node* node);

// Add a layout node to the layout tree being traversed.
void
add_layout_node(layout_traversal& traversal, layout_node* node);

// Record a change in the layout at the current position in the traversal.
void
record_layout_change(layout_traversal& traversal);

// detect_layout_change(ctx, value_storage, new_value) detects if new_value is
// different from the value stored in value_storage.
// If it is, it records the change in the layout tree and updates the stored
// value.
// The return value indicates whether or not a change was detected.
template<class Context, class T>
bool
detect_layout_change(Context& ctx, T* value_storage, T const& new_value)
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

// Resolve an absolute length into an actual length, in pixels.
float
resolve_absolute_length(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    unsigned axis,
    absolute_length const& length);
// 2D version
vector<2, float>
resolve_absolute_size(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    absolute_size const& size);
// Same as above, but the PPI an style_info are taken from the context.
float
resolve_absolute_length(
    layout_traversal& traversal, unsigned axis, absolute_length const& length);
// 2D version
vector<2, float>
resolve_absolute_size(layout_traversal& traversal, absolute_size const& size);

// Resolve a relative length into an actual length, in pixels.
float
resolve_relative_length(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    unsigned axis,
    relative_length const& length,
    float full_length);
// 2D version
vector<2, float>
resolve_relative_size(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    relative_size const& size,
    vector<2, float> const& full_size);
// Same as above, but the PPI an style_info are taken from the context.
float
resolve_relative_length(
    layout_traversal& traversal,
    unsigned axis,
    relative_length const& length,
    float full_length);
// 2D version
vector<2, float>
resolve_relative_size(
    layout_traversal& traversal,
    relative_size const& size,
    vector<2, float> const& full_size);

// Resolve a box_border_width into actual widths, in pixels.
box_border_width<float>
resolve_box_border_width(
    layout_traversal& traversal,
    box_border_width<absolute_length> const& border);

// Convert a resolved box_border_width to layout_scalars, rounding up.
box_border_width<layout_scalar>
as_layout_size(box_border_width<float> const& border);

// Add two box_border_widths.
template<class Scalar>
box_border_width<Scalar>
operator+(box_border_width<Scalar> const& a, box_border_width<Scalar> const& b)
{
    box_border_width<Scalar> sum;
    sum.top = a.top + b.top;
    sum.right = a.right + b.right;
    sum.bottom = a.bottom + b.bottom;
    sum.left = a.left + b.left;
    return sum;
}
template<class Scalar>
box_border_width<Scalar>&
operator+=(box_border_width<Scalar>& a, box_border_width<Scalar> const& b)
{
    a.top += b.top;
    a.right += b.right;
    a.bottom += b.bottom;
    a.left += b.left;
    return a;
}

// Get the box that results from adding a border to another box.
template<class Scalar>
box<2, Scalar>
add_border(box<2, Scalar> const& box, box_border_width<Scalar> const& border)
{
    return alia::box<2, Scalar>(
        box.corner - make_vector(border.left, border.top),
        box.size
            + make_vector(
                border.left + border.right, border.top + border.bottom));
}
// Get the box that results from removing a border from another box.
template<class Scalar>
box<2, Scalar>
remove_border(
    box<2, Scalar> const& box, box_border_width<Scalar> const& border)
{
    return alia::box<2, Scalar>(
        box.corner + make_vector(border.left, border.top),
        box.size
            - make_vector(
                border.left + border.right, border.top + border.bottom));
}

// The layout of a UI element is also dependent on the calculated minimum
// requirements for that element.
struct calculated_layout_requirements
{
    layout_scalar size;

    // The minimum space required on either side of the baseline.
    layout_scalar ascent, descent;

    calculated_layout_requirements()
    {
    }

    calculated_layout_requirements(
        layout_scalar size, layout_scalar ascent, layout_scalar descent)
        : size(size), ascent(ascent), descent(descent)
    {
    }
};

// Update 'current' so that it includes the additional requirements specified
// by 'additional'.
void
fold_in_requirements(
    calculated_layout_requirements& current,
    layout_requirements const& additional);

// Given a resolved layout spec from the application, and the calculated
// requirements of the UI element, this fills in the actual requirements for
// the UI element along the selected axis.
void
resolve_requirements(
    layout_requirements& requirements,
    resolved_layout_spec const& spec,
    unsigned axis,
    calculated_layout_requirements const& calculated);

// A UI element may be assigned more width than it needs, so this will
// determine the amount that it should actually use, based on its spec and
// requirements.
layout_scalar
resolve_assigned_width(
    resolved_layout_spec const& spec,
    layout_scalar assigned_width,
    layout_requirements const& horizontal_requirements);

// Again, a UI element may be assigned a bigger region than it requires, so
// this will determine what portion of the region the element should use,
// based on its spec and requirements.
relative_layout_assignment
resolve_relative_assignment(
    resolved_layout_spec const& spec,
    relative_layout_assignment const& assignment,
    layout_requirements const& horizontal_requirements,
    layout_requirements const& vertical_requirements);

inline bool
cache_is_fully_invalid(layout_cacher const& cacher)
{
    return cacher.bits == 0;
}

inline bool
cache_is_fully_valid(layout_cacher const& cacher)
{
    return cacher.bits == 7;
}

inline void
invalidate_cached_layout(layout_cacher& cacher)
{
    cacher.bits = 0;
}

bool
update_layout_cacher(
    layout_traversal& traversal,
    layout_cacher& cacher,
    layout const& layout_spec,
    layout_flag_set default_flags);

template<class Calculator>
layout_requirements const&
cache_horizontal_layout_requirements(
    layout_cacher& cacher, Calculator&& calculator)
{
    if (!read_bit(cacher.bits, layout_cacher::horizontal_query_results_valid))
    {
        resolve_requirements(
            cacher.horizontal_requirements,
            cacher.resolved_spec,
            0,
            std::forward<Calculator>(calculator)());
        set_bit(cacher.bits, layout_cacher::horizontal_query_results_valid);
    }
    return cacher.horizontal_requirements;
}

template<class Calculator>
layout_requirements const&
cache_vertical_layout_requirements(
    layout_cacher& cacher,
    layout_scalar assigned_width,
    Calculator&& calculator)
{
    if (cacher.assigned_width != assigned_width
        || !read_bit(cacher.bits, layout_cacher::vertical_query_results_valid))
    {
        resolve_requirements(
            cacher.vertical_requirements,
            cacher.resolved_spec,
            1,
            std::forward<Calculator>(calculator)());
        set_bit(cacher.bits, layout_cacher::vertical_query_results_valid);
        cacher.assigned_width = assigned_width;
    }
    return cacher.vertical_requirements;
}

template<class Assigner>
void
update_relative_assignment(
    layout_node& node,
    layout_cacher& cacher,
    relative_layout_assignment const& assignment,
    Assigner&& assigner)
{
    if (!read_bit(cacher.bits, layout_cacher::assignment_valid)
        || cacher.relative_assignment != assignment)
    {
        auto resolved_assignment = resolve_relative_assignment(
            cacher.resolved_spec,
            assignment,
            cacher.horizontal_requirements,
            node.get_vertical_requirements(assignment.region.size[0]));
        if (!read_bit(cacher.bits, layout_cacher::assignment_valid)
            || cacher.resolved_relative_assignment.region.size
                   != resolved_assignment.region.size
            || cacher.resolved_relative_assignment.baseline_y
                   != resolved_assignment.baseline_y)
        {
            std::forward<Assigner>(assigner)(resolved_assignment);
            set_bit(cacher.bits, layout_cacher::assignment_valid);
        }
        cacher.resolved_relative_assignment = resolved_assignment;
        cacher.relative_assignment = assignment;
    }
}

// Get the resolved relative assignment for a layout cacher.
inline relative_layout_assignment const&
get_assignment(layout_cacher const& cacher)
{
    return cacher.resolved_relative_assignment;
}

// The vast majority of layout containers behave identically except for the
// logic they use to calculate their requirements and divide their space
// amongst their children.
// All the shared behavior is refactored into simple_layout_container.
template<class Logic>
struct simple_layout_container : layout_container
{
    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override
    {
        return cache_horizontal_layout_requirements(cacher, [&] {
            return logic.get_horizontal_requirements(children);
        });
    }

    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        return cache_vertical_layout_requirements(cacher, assigned_width, [&] {
            return logic.get_vertical_requirements(
                children,
                resolve_assigned_width(
                    this->cacher.resolved_spec,
                    assigned_width,
                    this->get_horizontal_requirements()));
        });
    }

    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        update_relative_assignment(
            *this, cacher, assignment, [&](auto const& resolved_assignment) {
                this->assigned_size = resolved_assignment.region.size;
                logic.set_relative_assignment(
                    children,
                    resolved_assignment.region.size,
                    resolved_assignment.baseline_y);
            });
    }

    Logic logic;

    layout_vector assigned_size;
};

// get_simple_layout_container is a utility function for retrieving a
// simple_layout_container with a specific type of logic from a UI context's
// data graph and refreshing it.
template<class Logic>
void
get_simple_layout_container(
    layout_traversal& traversal,
    data_traversal& data,
    simple_layout_container<Logic>** container,
    Logic** logic,
    layout const& layout_spec)
{
    get_cached_data(data, container);

    if (is_refresh_pass(traversal))
    {
        update_layout_cacher(
            traversal, (*container)->cacher, layout_spec, FILL | UNPADDED);
    }

    *logic = &(*container)->logic;
}

// layout_leaf is used to represent simple leaves in the layout tree.
// Each refresh pass, the associated widget function calls refresh_layout(...)
// to pass along the user's layout_spec, the calculated properties of the
// widget, and the default layout flags.
// Then, during subsequent passes, the widget function can retrieve its
// layout assignment by calling assignment().
struct leaf_layout_requirements
{
    leaf_layout_requirements()
    {
    }
    leaf_layout_requirements(
        layout_vector const& size,
        layout_scalar ascent = 0,
        layout_scalar descent = 0)
        : size(size), ascent(ascent), descent(descent)
    {
    }
    layout_vector size;
    layout_scalar ascent, descent;
};
bool
operator==(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b);
bool
operator!=(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b);
struct layout_leaf : layout_node
{
    layout_leaf()
    {
    }

    void
    refresh_layout(
        layout_traversal& traversal,
        layout const& layout_spec,
        leaf_layout_requirements const& requirements,
        layout_flag_set default_flags = TOP | LEFT | PADDED);

    relative_layout_assignment const&
    assignment() const
    {
        return relative_assignment_;
    }

    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements();
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width);
    void
    set_relative_assignment(relative_layout_assignment const& assignment);

 private:
    // the resolved spec
    resolved_layout_spec resolved_spec_;

    // requirements supplied by the widget
    leaf_layout_requirements requirements_;

    // resolved relative assignment
    alia::relative_layout_assignment relative_assignment_;
};

// Some macros for implementing simple layout containers.

#define ALIA_DECLARE_LAYOUT_LOGIC_WITH_DATA(logic_type, data)                 \
    struct logic_type                                                         \
    {                                                                         \
        calculated_layout_requirements                                        \
        get_horizontal_requirements(layout_node* children);                   \
        calculated_layout_requirements                                        \
        get_vertical_requirements(                                            \
            layout_node* children, layout_scalar assigned_width);             \
        void                                                                  \
        set_relative_assignment(                                              \
            layout_node* children,                                            \
            layout_vector const& assigned_size,                               \
            layout_scalar assigned_baseline_y);                               \
        data                                                                  \
    };

#define ALIA_DECLARE_LAYOUT_LOGIC(logic_type)                                 \
    ALIA_DECLARE_LAYOUT_LOGIC_WITH_DATA(logic_type, )

#define ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(logic_type)                        \
    logic_type* logic;                                                        \
    get_simple_layout_container(                                              \
        traversal, data, &container_, &logic, layout_spec);                   \
    slc_.begin(traversal, container_);                                        \
    begin_layout_transform(transform_, traversal, container_->cacher)

// Various utilities for working with layout children...

// Walk through the children of a layout container.
template<class Visitor>
void
walk_layout_children(layout_node* children, Visitor&& visitor)
{
    for (layout_node* child = children; child; child = child->next)
        std::forward<Visitor>(visitor)(*child);
}

// Get the required width of the widest child in the list.
layout_scalar
get_max_child_width(layout_node* children);

// Get the horizontal requirements of all the children in the list, fold them
// together, and return the result.
calculated_layout_requirements
fold_horizontal_child_requirements(layout_node* children);

// Get the vertical requirements of all the children in the list, fold them
// together, and return the result.
calculated_layout_requirements
fold_vertical_child_requirements(
    layout_node* children, layout_scalar assigned_width);

// Assign the same layout region to all children in the list.
void
assign_identical_child_regions(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y);

// Get the total height of all children in the list.
layout_scalar
compute_total_height(layout_node* children, layout_scalar assigned_width);

} // namespace alia

#endif
