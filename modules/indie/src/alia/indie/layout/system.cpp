#include <alia/indie/layout/system.hpp>
#include <alia/indie/layout/utilities.hpp>

namespace alia {

static void
initialize_traversal(
    layout_system& system,
    layout_traversal& traversal,
    bool is_refresh,
    geometry_context* geometry,
    layout_style_info* style,
    vector<2, float> const& ppi)
{
    traversal.system = &system;
    traversal.active_container = 0;
    traversal.next_ptr = &system.root_node;
    traversal.is_refresh_pass = is_refresh;
    traversal.refresh_counter = system.refresh_counter;
    traversal.geometry = geometry;
    traversal.style_info = style;
    traversal.ppi = ppi;

    style->font_size = 0;
    style->character_size = make_vector<layout_scalar>(0, 0);
    style->x_height = 0;
    style->padding_size = make_layout_vector(4, 4);
    style->magnification = 1;
}
void
scoped_layout_traversal::begin(
    layout_system& system,
    layout_traversal& traversal,
    geometry_context& geometry,
    vector<2, float> const& ppi)
{
    initialize_traversal(
        system, traversal, false, &geometry, &dummy_style_info_, ppi);
}
void
scoped_layout_refresh::begin(
    layout_system& system,
    layout_traversal& traversal,
    vector<2, float> const& ppi)
{
    initialize_traversal(system, traversal, true, 0, &dummy_style_info_, ppi);
}

void
resolve_layout(layout_node* root_node, layout_vector const& size)
{
    if (root_node)
    {
        get_horizontal_requirements(*root_node);
        layout_requirements y = get_vertical_requirements(*root_node, size[0]);
        set_relative_assignment(
            *root_node,
            relative_layout_assignment(
                layout_box(make_layout_vector(0, 0), size), y.ascent));
    }
}

void
resolve_layout(layout_system& system, layout_vector const& size)
{
    resolve_layout(system.root_node, size);
    // Increment the refresh counter immediately after resolving layout so
    // that any changes detected after this will be associated with the new
    // counter value and thus cause a recalculation.
    ++system.refresh_counter;
}

layout_vector
get_minimum_size(layout_node* root_node)
{
    if (root_node)
    {
        layout_requirements horizontal
            = get_horizontal_requirements(*root_node);
        layout_requirements vertical
            = get_vertical_requirements(*root_node, horizontal.size);
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
