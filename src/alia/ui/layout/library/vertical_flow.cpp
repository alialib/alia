#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

// The vertical flow algorithm is a bit simplistic. All columns have the
// same width.

calculated_layout_requirements
vertical_flow_layout_logic::get_horizontal_requirements(layout_node* children)
{
    // In the worst case scenario, we can put all children in one column,
    // so the required width is simply the width required by the widest
    // child.
    return fold_horizontal_child_requirements(children);
}

calculated_layout_requirements
vertical_flow_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    layout_scalar column_width = get_max_child_width(children);

    layout_scalar total_height = compute_total_height(children, column_width);

    int n_columns = (std::max)(int(assigned_width / column_width), 1);

    layout_scalar average_column_height = total_height / n_columns;

    // Break the children into columns and determine the longest column
    // size. Each column accumulates children until it's at least as big as
    // the average height. There are other strategies that would yield
    // smaller and more balanced placements, but this one is simple and
    // gives reasonable results.
    layout_scalar max_column_height = 0;
    layout_scalar current_column_height = 0;
    int column_index = 0;
    walk_layout_children(children, [&](layout_node& node) {
        if (current_column_height >= average_column_height)
        {
            if (current_column_height > max_column_height)
                max_column_height = current_column_height;
            ++column_index;
            current_column_height = 0;
        }
        layout_requirements y = node.get_vertical_requirements(column_width);
        current_column_height += y.size;
    });

    return calculated_layout_requirements(max_column_height, 0, 0);
}

void
vertical_flow_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar /*assigned_baseline_y*/)
{
    layout_scalar column_width = get_max_child_width(children);

    layout_scalar total_height = compute_total_height(children, column_width);

    int n_columns = (std::max)(int(assigned_size[0] / column_width), 1);

    layout_scalar average_column_height = total_height / n_columns;

    // Break the children into columns and assign their regions.
    // Each column accumulates children until it's at least as big as the
    // average height. There are other strategies that would yield smaller
    // and more balanced placements, but this one is simple and gives
    // reasonable results.
    layout_vector p = make_layout_vector(0, 0);
    int column_index = 0;
    walk_layout_children(children, [&](layout_node& node) {
        if (p[1] >= average_column_height)
        {
            ++column_index;
            p[0] += column_width;
            p[1] = 0;
        }
        layout_requirements y = node.get_vertical_requirements(column_width);
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(p, make_layout_vector(column_width, y.size)),
            y.ascent});
        p[1] += y.size;
    });
}

void
vertical_flow_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(vertical_flow_layout_logic);
}

} // namespace alia
