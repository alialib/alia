#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

calculated_layout_requirements
rotated_layout_logic::get_horizontal_requirements(layout_node* children)
{
    // The horizontal requirements of the rotated layout are the vertical
    // requirements of the child, but in order to obtain the vertical
    // requirements, we need to specify a width, so we specify an essentially
    // infinite width, which should give the minimum required height of the
    // child.
    layout_scalar width = 0;
    walk_layout_children(children, [&](layout_node& node) {
        // The layout protocol requires that we ask for horizontal
        // requirements first.
        (void) node.get_horizontal_requirements();
        auto y = node.get_vertical_requirements(100000);
        width = (std::max)(y.size, width);
    });
    return calculated_layout_requirements(width, 0, 0);
}

calculated_layout_requirements
rotated_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    // This requires performing the inverse of the normal vertical
    // requirements request (i.e., determine the minimum width required for
    // a given height), so we do a binary search to determine the minimum
    // child width for which the child height is less than or equal to
    // our assigned width.
    layout_scalar height = 0;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements x = node.get_horizontal_requirements();
        layout_scalar lower_bound = x.size, upper_bound = 100000;
        while (!layout_scalars_almost_equal(upper_bound, lower_bound))
        {
            layout_scalar test_value = (upper_bound + lower_bound) / 2;
            layout_requirements y = node.get_vertical_requirements(test_value);
            if (y.size <= assigned_width)
                upper_bound = test_value;
            else
                lower_bound = test_value + layout_scalar_epsilon;
        }
        height = (std::max)(upper_bound, height);
    });
    return calculated_layout_requirements(height, 0, 0);
}

void
rotated_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar /* assigned_baseline_y */)
{
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements y
            = node.get_vertical_requirements(assigned_size[1]);
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(
                make_layout_vector(0, 0),
                make_layout_vector(assigned_size[1], assigned_size[0])),
            y.ascent});
    });
}

void
rotated_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& layout_spec)
{
    rotated_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container_, &logic, layout_spec);
    slc_.begin(traversal, container_);

    if (!traversal.is_refresh_pass)
    {
        transform_.begin(*traversal.geometry);
        transform_.set(
            translation_matrix(vector<2, double>(
                get_assignment(container_->cacher).region.corner
                + make_layout_vector(
                    0, get_assignment(container_->cacher).region.size[1])))
            * make_matrix<double>(0, 1, 0, -1, 0, 0, 0, 0, 1));
    }
}

} // namespace alia
