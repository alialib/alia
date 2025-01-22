#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

void
initialize_layout_traversal(
    layout_system& system,
    layout_traversal& traversal,
    bool is_refresh,
    geometry_context* geometry,
    layout_style_info const* style,
    vector<2, float> const& ppi)
{
    traversal.system = &system;
    traversal.active_container = 0;
    traversal.next_ptr = &system.root_node;
    traversal.is_refresh_pass = is_refresh;
    traversal.geometry = geometry;
    traversal.style_info = style;
    traversal.ppi = ppi;
}

void
resolve_layout(layout_node* root_node, layout_box const& assignment)
{
    if (root_node)
    {
        layout_requirements y
            = root_node->get_vertical_requirements(assignment.size[0]);
        root_node->set_relative_assignment(
            relative_layout_assignment{assignment, y.ascent});
    }
}

void
resolve_layout(layout_system& system, layout_box const& assignment)
{
    resolve_layout(system.root_node, assignment);
}

layout_vector
get_minimum_size(layout_node* root_node)
{
    if (root_node)
    {
        layout_requirements horizontal
            = root_node->get_horizontal_requirements();
        layout_requirements vertical
            = root_node->get_vertical_requirements(horizontal.size);
        return make_layout_vector(horizontal.size, vertical.size);
    }
    else
        return make_layout_vector(0, 0);
}

layout_vector
get_minimum_size(layout_system& system)
{
    return get_minimum_size(system.root_node);
}

} // namespace alia
