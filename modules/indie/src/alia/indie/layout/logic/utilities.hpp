#ifndef ALIA_INDIE_LAYOUT_LOGIC_UTILITIES_HPP
#define ALIA_INDIE_LAYOUT_LOGIC_UTILITIES_HPP

#include <alia/indie/layout/specification.hpp>

namespace alia { namespace indie {

template<class Children>
layout_scalar
get_max_child_width(Children&& children)
{
    layout_scalar width = 0;
    walk_layout_nodes(
        std::forward<Children>(children), [&](layout_node& node) {
            layout_requirements x = node.get_horizontal_requirements();
            width = (std::max)(x.size, width);
        });
    return width;
}

template<class Children>
calculated_layout_requirements
fold_horizontal_child_requirements(Children&& children)
{
    return calculated_layout_requirements{
        get_max_child_width(std::forward<Children>(children)), 0, 0};
}

template<class Children>
calculated_layout_requirements
fold_vertical_child_requirements(
    Children&& children, layout_scalar assigned_width)
{
    calculated_layout_requirements requirements{0, 0, 0};
    walk_layout_nodes(
        std::forward<Children>(children), [&](layout_node& node) {
            fold_in_requirements(
                requirements, node.get_vertical_requirements(assigned_width));
        });
    return requirements;
}

template<class Children>
void
assign_identical_child_regions(
    Children&& children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    walk_layout_nodes(
        std::forward<Children>(children), [&](layout_node& node) {
            node.set_relative_assignment(relative_layout_assignment{
                layout_box(make_layout_vector(0, 0), assigned_size),
                assigned_baseline_y});
        });
}

template<class Children>
layout_scalar
compute_total_height(Children&& children, layout_scalar assigned_width)
{
    layout_scalar total_height = 0;
    walk_layout_nodes(
        std::forward<Children>(children), [&](layout_node& node) {
            layout_requirements y
                = node.get_vertical_requirements(assigned_width);
            total_height += y.size;
        });
    return total_height;
}

}} // namespace alia::indie

#endif
