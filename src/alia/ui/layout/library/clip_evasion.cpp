#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

calculated_layout_requirements
clip_evasion_layout_logic::get_horizontal_requirements(layout_node* children)
{
    return fold_horizontal_child_requirements(children);
}

calculated_layout_requirements
clip_evasion_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    return fold_vertical_child_requirements(children, assigned_width);
}

void
clip_evasion_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    assign_identical_child_regions(
        children, assigned_size, assigned_baseline_y);
}

void
clip_evasion_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& layout_spec)
{
    clip_evasion_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container_, &logic, layout_spec);
    slc_.begin(traversal, container_);

    if (!traversal.is_refresh_pass)
    {
        vector<2, double> corner_on_surface = transform(
            traversal.geometry->transformation_matrix,
            vector<2, double>(
                get_assignment(container_->cacher).region.corner));
        vector<2, double> high_corner_on_surface = transform(
            traversal.geometry->transformation_matrix,
            vector<2, double>(
                get_high_corner(get_assignment(container_->cacher).region)));
        vector<2, double> offset;
        for (unsigned i = 0; i != 2; ++i)
        {
            // If the content is smaller than the clip region, just prevent it
            // from scrolling off the top of the clip region.
            if (high_corner_on_surface[i] - corner_on_surface[i]
                < traversal.geometry->clip_region.size[i])
            {
                offset[i] = (std::max)(
                    0.,
                    traversal.geometry->clip_region.corner[i]
                        - corner_on_surface[i]);
            }
            // Otherwise, just make sure that it's not scrolled to the point
            // that there's empty space.
            else if (
                corner_on_surface[i]
                > traversal.geometry->clip_region.corner[i])
            {
                offset[i] = traversal.geometry->clip_region.corner[i]
                            - corner_on_surface[i];
            }
            else if (
                high_corner_on_surface[i]
                < get_high_corner(traversal.geometry->clip_region)[i])
            {
                offset[i] = get_high_corner(traversal.geometry->clip_region)[i]
                            - high_corner_on_surface[i];
            }
            else
            {
                offset[i] = 0;
            }
        }
        transform_.begin(*traversal.geometry);
        transform_.set(translation_matrix(offset));
    }
}

} // namespace alia
