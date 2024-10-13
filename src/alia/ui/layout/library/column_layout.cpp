#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

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
column_layout_logic::get_horizontal_requirements(layout_node* children)
{
    return fold_horizontal_child_requirements(children);
}

calculated_layout_requirements
column_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    layout_scalar total_height = 0;
    layout_scalar ascent = 0, descent = 0;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements r = node.get_vertical_requirements(assigned_width);
        total_height += r.size;
        // The ascent of a column is the ascent of its first child.
        // Its descent is the descent of its first child plus the total
        // height of all other children. However, if the first child
        // doesn't care about the baseline, then the column ignores it as
        // well.
        if (&node == children)
        {
            ascent = r.ascent;
            descent = r.descent;
        }
        else if (ascent != 0 || descent != 0)
            descent += r.size;
    });
    return calculated_layout_requirements(total_height, ascent, descent);
}

void
column_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_scalar total_size = 0;
    float total_growth = 0;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements x
            = node.get_vertical_requirements(assigned_size[0]);
        total_size += x.size;
        total_growth += x.growth_factor;
    });
    layout_vector p = make_layout_vector(0, 0);
    layout_scalar remaining_extra_size = assigned_size[1] - total_size;
    float remaining_growth = total_growth;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements y
            = node.get_vertical_requirements(assigned_size[0]);
        layout_scalar this_height = calculate_child_size(
            remaining_extra_size, remaining_growth, y.size, y.growth_factor);
        // If this is the first child, assign it the baseline of the
        // column. The exception to this rule is when the column was
        // allocated more space than it requested. In this case, the
        // baseline calculation may have been inconsistent with how the
        // column is planning to allocate the extra space.
        layout_scalar this_baseline
            = &node == children && remaining_extra_size == 0
                  ? assigned_baseline_y
                  : y.ascent;
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(p, make_layout_vector(assigned_size[0], this_height)),
            this_baseline});
        p[1] += this_height;
    });
}

void
column_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    simple_layout_container<column_layout_logic>* container,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(column_layout_logic);
}

} // namespace alia
