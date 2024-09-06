#include <alia/ui/layout/grids.hpp>

#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

void
do_spacer(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& layout_spec)
{
    layout_leaf* node;
    get_cached_data(data, &node);

    if (traversal.is_refresh_pass)
    {
        node->refresh_layout(
            traversal,
            layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(traversal, node);
    }
}

void
do_spacer(
    layout_traversal& traversal,
    data_traversal& data,
    layout_box* region,
    layout const& layout_spec)
{
    layout_leaf* node;
    get_cached_data(data, &node);

    if (traversal.is_refresh_pass)
    {
        node->refresh_layout(
            traversal,
            layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(traversal, node);
    }
    else
        *region = node->assignment().region;
}
} // namespace alia
