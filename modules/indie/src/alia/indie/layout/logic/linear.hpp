#ifndef ALIA_INDIE_LAYOUT_LOGIC_LINEAR_HPP
#define ALIA_INDIE_LAYOUT_LOGIC_LINEAR_HPP

#include <alia/indie/layout/logic/utilities.hpp>
#include <alia/indie/layout/specification.hpp>

namespace alia { namespace indie {

template<class Children>
void
compute_total_width_and_growth(
    layout_scalar* total_width, float* total_growth, Children&& children)
{
    *total_width = 0;
    *total_growth = 0;
    walk_layout_nodes(
        std::forward<Children>(children), [&](layout_node_interface& node) {
            layout_requirements r = node.get_horizontal_requirements();
            *total_width += r.size;
            *total_growth += r.growth_factor;
        });
}

inline layout_scalar
calculate_child_size(
    layout_scalar& remaining_extra_size,
    float& remaining_growth,
    layout_scalar this_required_size,
    float this_growth)
{
    if (remaining_growth != 0)
    {
        layout_scalar extra_size = round_to_layout_scalar(
            float(remaining_extra_size) * (this_growth / remaining_growth));
        remaining_extra_size -= extra_size;
        remaining_growth -= this_growth;
        return this_required_size + extra_size;
    }
    else
    {
        return this_required_size;
    }
}

struct row_layout_logic
{
    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        layout_scalar total_size;
        float total_growth;
        compute_total_width_and_growth(
            &total_size, &total_growth, std::forward<Children>(children));
        return calculated_layout_requirements{total_size, 0, 0};
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        layout_scalar total_size;
        float total_growth;
        compute_total_width_and_growth(
            &total_size, &total_growth, std::forward<Children>(children));
        layout_scalar remaining_extra_size = assigned_width - total_size;
        float remaining_growth = total_growth;
        calculated_layout_requirements requirements{0, 0, 0};
        walk_layout_nodes(children, [&](layout_node_interface& node) {
            layout_requirements x = node.get_horizontal_requirements();
            layout_scalar this_width = calculate_child_size(
                remaining_extra_size,
                remaining_growth,
                x.size,
                x.growth_factor);
            fold_in_requirements(
                requirements, node.get_vertical_requirements(this_width));
        });
        return requirements;
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        layout_scalar total_size;
        float total_growth;
        compute_total_width_and_growth(
            &total_size, &total_growth, std::forward<Children>(children));
        layout_vector p = make_layout_vector(0, 0);
        layout_scalar remaining_extra_size = assigned_size[0] - total_size;
        float remaining_growth = total_growth;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                layout_requirements x = node.get_horizontal_requirements();
                layout_scalar this_width = calculate_child_size(
                    remaining_extra_size,
                    remaining_growth,
                    x.size,
                    x.growth_factor);
                node.set_relative_assignment(relative_layout_assignment{
                    layout_box(
                        p, make_layout_vector(this_width, assigned_size[1])),
                    assigned_baseline_y});
                p[0] += this_width;
            });
    }
};

struct column_layout_logic
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
        layout_scalar total_height = 0;
        layout_scalar ascent = 0, descent = 0;
        bool is_first = true;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                layout_requirements r
                    = node.get_vertical_requirements(assigned_width);
                total_height += r.size;
                // The ascent of a column is the ascent of its first child.
                // Its descent is the descent of its first child plus the total
                // height of all other children. However, if the first child
                // doesn't care about the baseline, then the column ignores it
                // as well.
                if (is_first)
                {
                    ascent = r.ascent;
                    descent = r.descent;
                    is_first = false;
                }
                else if (ascent != 0 || descent != 0)
                {
                    descent += r.size;
                }
            });
        return calculated_layout_requirements{total_height, ascent, descent};
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        layout_scalar total_size = 0;
        float total_growth = 0;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                layout_requirements x
                    = node.get_vertical_requirements(assigned_size[0]);
                total_size += x.size;
                total_growth += x.growth_factor;
            });
        layout_vector p = make_layout_vector(0, 0);
        layout_scalar remaining_extra_size = assigned_size[1] - total_size;
        float remaining_growth = total_growth;
        bool is_first = true;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                layout_requirements y
                    = node.get_vertical_requirements(assigned_size[0]);
                layout_scalar this_height = calculate_child_size(
                    remaining_extra_size,
                    remaining_growth,
                    y.size,
                    y.growth_factor);
                // If this is the first child, assign it the baseline of the
                // column. The exception to this rule is when the column was
                // allocated more space than it requested. In this case, the
                // baseline calculation may have been inconsistent with how the
                // column is planning to allocate the extra space.
                layout_scalar this_baseline
                    = is_first && remaining_extra_size == 0
                          ? assigned_baseline_y
                          : y.ascent;
                node.set_relative_assignment(relative_layout_assignment{
                    layout_box(
                        p, make_layout_vector(assigned_size[0], this_height)),
                    this_baseline});
                p[1] += this_height;
                is_first = false;
            });
    }
};

}} // namespace alia::indie

#endif
