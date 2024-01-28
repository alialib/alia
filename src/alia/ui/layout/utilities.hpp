#ifndef ALIA_UI_LAYOUT_UTILITIES_HPP
#define ALIA_UI_LAYOUT_UTILITIES_HPP

#include <alia/ui/layout/node_interface.hpp>
#include <alia/ui/widget.hpp>

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
    return layout_scalar(std::ceil(x));
}
inline layout_scalar
as_layout_size(float x)
{
    return layout_scalar(std::ceil(x));
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

// Initialize a container for use within the given context.
inline void
initialize(
    layout_traversal<widget_container, widget>& traversal,
    widget_container& container)
{
    container.last_content_change = traversal.refresh_counter;
}

// Record a change in the layout at the current position in the traversal.
inline void
record_layout_change(layout_traversal<widget_container, widget>& traversal)
{
    if (traversal.active_container)
        traversal.active_container->record_content_change(traversal);
}

inline void
set_next_node(
    layout_traversal<widget_container, widget>& traversal, widget* node)
{
    if (*traversal.next_ptr != node)
    {
        record_layout_change(traversal);
        *traversal.next_ptr = node;
    }
}

// Add a layout node to the layout tree being traversed.
inline void
add_layout_node(
    layout_traversal<widget_container, widget>& traversal, widget* node)
{
    set_next_node(traversal, node);
    traversal.next_ptr = &node->next;
    node->parent = traversal.active_container;
}

// detect_layout_change(ctx, value_storage, new_value) detects if new_value is
// different from the value stored in value_storage.
// If it is, it records the change in the layout tree and updates the stored
// value.
// The return value indicates whether or not a change was detected.
template<class Traversal, class T>
bool
detect_layout_change(
    Traversal& traversal, T* value_storage, T const& new_value)
{
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
template<class Traversal>
float
resolve_absolute_length(
    Traversal const& traversal, unsigned axis, absolute_length const& length)
{
    return resolve_absolute_length(
        traversal.ppi, *traversal.style_info, axis, length);
}
// 2D version
template<class Traversal>
vector<2, float>
resolve_absolute_size(Traversal& traversal, absolute_size const& size)
{
    return resolve_absolute_size(traversal.ppi, *traversal.style_info, size);
}

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
template<class Traversal>
float
resolve_relative_length(
    Traversal& traversal,
    unsigned axis,
    relative_length const& length,
    float full_length)
{
    return resolve_relative_length(
        traversal.ppi, *traversal.style_info, axis, length, full_length);
}
// 2D version
template<class Traversal>
vector<2, float>
resolve_relative_size(
    Traversal& traversal,
    relative_size const& size,
    vector<2, float> const& full_size)
{
    return resolve_relative_size(
        traversal.ppi, *traversal.style_info, size, full_size);
}

// Resolve a box_border_width into actual widths, in pixels.
template<class Traversal>
box_border_width<float>
resolve_box_border_width(
    Traversal& traversal, box_border_width<absolute_length> const& border)
{
    return box_border_width<float>(
        resolve_absolute_length(traversal, 1, border.top),
        resolve_absolute_length(traversal, 0, border.right),
        resolve_absolute_length(traversal, 1, border.bottom),
        resolve_absolute_length(traversal, 0, border.left));
}

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
add_border(
    alia::box<2, Scalar> const& box, box_border_width<Scalar> const& border)
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
    layout_traversal<widget_container, widget>& traversal,
    resolved_layout_spec& resolved,
    layout const& spec,
    layout_flag_set default_flags);

// The layout of a UI element is also dependent on the calculated minimum
// requirements for that element.
struct calculated_layout_requirements
{
    layout_scalar size;

    // the minimum space required on either side of the baseline
    layout_scalar ascent, descent;
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
inline bool
operator==(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b)
{
    return a.size == b.size && a.ascent == b.ascent && a.descent == b.descent;
}
inline bool
operator!=(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b)
{
    return !(a == b);
}
struct layout_leaf : widget
{
    layout_leaf()
    {
    }

    void
    refresh_layout(
        layout_traversal<widget_container, widget>& traversal,
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
    relative_layout_assignment relative_assignment_;
};

// Various utilities for working with layout children...

// Walk through the children of a layout container.
template<class Visitor>
void
walk_layout_nodes(widget* children, Visitor&& visitor)
{
    for (widget* child = children; child; child = child->next)
        std::forward<Visitor>(visitor)(*child);
}

} // namespace alia

#endif
