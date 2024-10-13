#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

namespace {

void
compute_total_width_and_growth(
    layout_scalar* total_width, float* total_growth, layout_node* children)
{
    *total_width = 0;
    *total_growth = 0;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements r = node.get_horizontal_requirements();
        *total_width += r.size;
        *total_growth += r.growth_factor;
    });
}

} // namespace

calculated_layout_requirements
row_layout_logic::get_horizontal_requirements(layout_node* children)
{
    layout_scalar total_size;
    float total_growth;
    compute_total_width_and_growth(&total_size, &total_growth, children);
    return calculated_layout_requirements(total_size, 0, 0);
}

// TODO: Deduplicate.
static layout_scalar
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
        return this_required_size;
}

calculated_layout_requirements
row_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    layout_scalar total_size;
    float total_growth;
    compute_total_width_and_growth(&total_size, &total_growth, children);
    layout_scalar remaining_extra_size = assigned_width - total_size;
    float remaining_growth = total_growth;
    calculated_layout_requirements requirements(0, 0, 0);
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements x = node.get_horizontal_requirements();
        layout_scalar this_width = calculate_child_size(
            remaining_extra_size, remaining_growth, x.size, x.growth_factor);
        fold_in_requirements(
            requirements, node.get_vertical_requirements(this_width));
    });
    return requirements;
}

void
row_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_scalar total_size;
    float total_growth;
    compute_total_width_and_growth(&total_size, &total_growth, children);
    layout_vector p = make_layout_vector(0, 0);
    layout_scalar remaining_extra_size = assigned_size[0] - total_size;
    float remaining_growth = total_growth;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements x = node.get_horizontal_requirements();
        layout_scalar this_width = calculate_child_size(
            remaining_extra_size, remaining_growth, x.size, x.growth_factor);
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(p, make_layout_vector(this_width, assigned_size[1])),
            assigned_baseline_y});
        p[0] += this_width;
    });
}

void
row_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    simple_layout_container<row_layout_logic>* container,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(row_layout_logic);
}

} // namespace alia
