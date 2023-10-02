#ifndef ALIA_INDIE_LAYOUT_LOGIC_CLAMPED_HPP
#define ALIA_INDIE_LAYOUT_LOGIC_CLAMPED_HPP

#include <alia/indie/layout/logic/utilities.hpp>
#include <alia/indie/layout/specification.hpp>

namespace alia { namespace indie {

struct clamped_layout_logic
{
    layout_vector max_size;

    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        calculated_layout_requirements requirements
            = fold_horizontal_child_requirements(
                std::forward<Children>(children));
        return requirements;
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        calculated_layout_requirements requirements
            = fold_horizontal_child_requirements(
                std::forward<Children>(children));
        layout_scalar clamped_width = (std::max)(
            requirements.size,
            this->max_size[0] <= 0
                ? assigned_width
                : (std::min)(assigned_width, this->max_size[0]));
        return fold_vertical_child_requirements(
            std::forward<Children>(children), clamped_width);
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar /*assigned_baseline_y*/)
    {
        layout_vector clamped_size;
        for (int i = 0; i != 2; ++i)
        {
            clamped_size[i]
                = this->max_size[i] <= 0
                      ? assigned_size[i]
                      : (std::min)(assigned_size[i], this->max_size[i]);
        }
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                layout_requirements y
                    = node.get_vertical_requirements(clamped_size[0]);
                node.set_relative_assignment(relative_layout_assignment{
                    layout_box(
                        (assigned_size - clamped_size) / 2, clamped_size),
                    y.ascent});
            });
    }
};

}} // namespace alia::indie

#endif
