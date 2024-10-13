#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

calculated_layout_requirements
layered_layout_logic::get_horizontal_requirements(layout_node* children)
{
    return fold_horizontal_child_requirements(children);
}

calculated_layout_requirements
layered_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    return fold_vertical_child_requirements(children, assigned_width);
}

void
layered_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    assign_identical_child_regions(
        children, assigned_size, assigned_baseline_y);
}

void
layered_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    simple_layout_container<layered_layout_logic>* container,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(layered_layout_logic);
}

} // namespace alia
