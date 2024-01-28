#ifndef ALIA_UI_LAYOUT_LOGIC_BORDERED_HPP
#define ALIA_UI_LAYOUT_LOGIC_BORDERED_HPP

#include <alia/ui/layout/logic/utilities.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

struct bordered_layout_logic
{
    box_border_width<layout_scalar> border;

    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        return calculated_layout_requirements{
            get_max_child_width(std::forward<Children>(children))
                + (border.left + border.right),
            0,
            0};
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        calculated_layout_requirements requirements
            = fold_vertical_child_requirements(
                std::forward<Children>(children),
                assigned_width - (border.left + border.right));
        return calculated_layout_requirements{
            requirements.size + border.top + border.bottom,
            requirements.ascent + border.top,
            requirements.descent + border.bottom};
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
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

} // namespace alia

#endif
