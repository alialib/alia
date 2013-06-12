#include <alia/layout/api.hpp>
#include <alia/layout/utilities.hpp>

namespace alia {

bool operator==(absolute_length const& a, absolute_length const& b)
{
    return a.length == b.length && a.units == b.units;
}
bool operator!=(absolute_length const& a, absolute_length const& b)
{
    return !(a == b);
}

bool operator==(relative_length const& a, relative_length const& b)
{
    return a.length == b.length && a.is_relative == b.is_relative &&
        (!a.is_relative || a.units == b.units);
}
bool operator!=(relative_length const& a, relative_length const& b)
{
    return !(a == b);
}

bool operator==(layout const& a, layout const& b)
{
    return a.size == b.size && a.flags == b.flags &&
        a.growth_factor == b.growth_factor;
}
bool operator!=(layout const& a, layout const& b)
{
    return !(a == b);
}

void scoped_layout_container::begin(
    layout_traversal& traversal, layout_container* container)
{
    if (traversal.is_refresh_pass)
    {
        traversal_ = &traversal;

        set_next_node(traversal, container);
        container->parent = traversal.active_container;

        traversal.next_ptr = &container->children;
        traversal.active_container = container;
    }
}
void scoped_layout_container::end()
{
    if (traversal_)
    {
        set_next_node(*traversal_, 0);

        layout_container* container = traversal_->active_container;
        traversal_->next_ptr = &container->next;
        traversal_->active_container = container->parent;

        traversal_ = 0;
    }
}

layout_box get_container_region(simple_layout_container const& container)
{
    return layout_box(make_layout_vector(0, 0), container.assigned_size);
}

layout_box get_padded_container_region(
    simple_layout_container const& container)
{
    return layout_box(
        container.cacher.relative_assignment.region.corner -
            container.cacher.resolved_relative_assignment.region.corner,
        container.cacher.relative_assignment.region.size);
}

layout_vector get_container_offset(simple_layout_container const& container)
{
    return get_assignment(container.cacher).region.corner;
}

}
