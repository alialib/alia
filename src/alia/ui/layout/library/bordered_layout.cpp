#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

calculated_layout_requirements
bordered_layout_logic::get_horizontal_requirements(layout_node* children)
{
    return calculated_layout_requirements(
        get_max_child_width(children) + (border.left + border.right), 0, 0);
}

calculated_layout_requirements
bordered_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    calculated_layout_requirements requirements
        = fold_vertical_child_requirements(
            children, assigned_width - (border.left + border.right));
    return calculated_layout_requirements(
        requirements.size + border.top + border.bottom,
        requirements.ascent + border.top,
        requirements.descent + border.bottom);
}

void
bordered_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    walk_layout_children(children, [&](layout_node& node) {
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

void
bordered_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    box_border_width<absolute_length> border,
    layout const& layout_spec)
{
    bordered_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container_, &logic, layout_spec);
    slc_.begin(traversal, container_);
    begin_layout_transform(transform_, traversal, container_->cacher);
    if (traversal.is_refresh_pass)
    {
        detect_layout_change(
            traversal,
            &logic->border,
            as_layout_size(resolve_box_border_width(traversal, border)));
    }
}

} // namespace alia
