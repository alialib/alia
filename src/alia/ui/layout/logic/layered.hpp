#ifndef ALIA_INDIE_LAYOUT_LOGIC_LAYERED_HPP
#define ALIA_INDIE_LAYOUT_LOGIC_LAYERED_HPP

#include <alia/indie/layout/logic/utilities.hpp>
#include <alia/indie/layout/specification.hpp>

namespace alia { namespace indie {

struct layered_layout_logic
{
    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        return fold_horizontal_child_requirements(
            std::forward<Children>(children));
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        return fold_vertical_child_requirements(
            std::forward<Children>(children), assigned_width);
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        assign_identical_child_regions(
            std::forward<Children>(children),
            assigned_size,
            assigned_baseline_y);
    }
};

}} // namespace alia::indie

#endif
