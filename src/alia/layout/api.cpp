#include <alia/layout/api.hpp>
#include <alia/layout/utilities.hpp>

namespace alia {

layout default_layout;

bool operator==(size const& a, size const& b)
{
    return a.width == b.width && a.height == b.height &&
        a.x_units == b.x_units && a.y_units == b.y_units;
}
bool operator!=(size const& a, size const& b)
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

}
