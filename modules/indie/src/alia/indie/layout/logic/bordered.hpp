#ifndef ALIA_INDIE_LAYOUT_LOGIC_BORDERED_HPP
#define ALIA_INDIE_LAYOUT_LOGIC_BORDERED_HPP

#include <alia/indie/layout/logic/utilities.hpp>
#include <alia/indie/layout/specification.hpp>

namespace alia { namespace indie {

struct bordered_layout_logic
{
    box_border_width<layout_scalar> border;

    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(layout_node* children)
    {
        return calculated_layout_requirements{
            get_max_child_width(children) + (border.left + border.right),
            0,
            0};
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        layout_node* children, layout_scalar assigned_width)
    {
        calculated_layout_requirements requirements
            = fold_vertical_child_requirements(
                children, assigned_width - (border.left + border.right));
        return calculated_layout_requirements{
            requirements.size + border.top + border.bottom,
            requirements.ascent + border.top,
            requirements.descent + border.bottom};
    }

    template<class Children>
    void
    set_relative_assignment(
        layout_node* children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        walk_layout_nodes(children, [&](layout_node& node) {
            node.set_relative_assignment(relative_layout_assignment{
                layout_box(
                    make_layout_vector(border.left, border.top),
                    assigned_size
                        - make_layout_vector(
                            border.left + border.right,
                            border.top + border.bottom)),
                assigned_baseline_y - border.top});
        });
    }
};

}} // namespace alia::indie

#endif
