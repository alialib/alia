#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

calculated_layout_requirements
clamped_layout_logic::get_horizontal_requirements(layout_node* children)
{
    calculated_layout_requirements requirements
        = fold_horizontal_child_requirements(children);
    return requirements;
}

calculated_layout_requirements
clamped_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    calculated_layout_requirements requirements
        = fold_horizontal_child_requirements(children);
    layout_scalar clamped_width = (std::max)(
        requirements.size,
        this->max_size[0] <= 0
            ? assigned_width
            : (std::min)(assigned_width, this->max_size[0]));
    return fold_vertical_child_requirements(children, clamped_width);
}

void
clamped_layout_logic::set_relative_assignment(
    layout_node* children,
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
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements y
            = node.get_vertical_requirements(clamped_size[0]);
        node.set_relative_assignment(relative_layout_assignment{
            layout_box((assigned_size - clamped_size) / 2, clamped_size),
            y.ascent});
    });
}

void
clamped_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    absolute_size max_size,
    layout const& layout_spec)
{
    clamped_layout_logic* logic;
    // TODO
    get_simple_layout_container(
        traversal, data, &container_, &logic, layout_spec);
    slc_.begin(traversal, container_);
    begin_layout_transform(transform_, traversal, container_->cacher);
    if (traversal.is_refresh_pass)
    {
        detect_layout_change(
            traversal,
            &logic->max_size,
            as_layout_size(resolve_absolute_size(traversal, max_size)));
    }
}

} // namespace alia
