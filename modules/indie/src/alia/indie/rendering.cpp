#include <alia/indie/rendering.hpp>
#include <alia/indie/system/object.hpp>

namespace alia { namespace indie {

void
render_children(SkCanvas& canvas, render_container& container)
{
    for (render_node* node = container.children; node; node = node->next)
        node->render(canvas);
}

void
add_render_node(render_traversal& traversal, render_node* node)
{
    *traversal.next_ptr = node;
    traversal.next_ptr = &node->next;
}

void
scoped_render_container::begin(
    render_traversal& traversal, render_container* container)
{
    traversal_ = &traversal;
    container_ = container;

    *traversal.next_ptr = container;
    traversal.next_ptr = &container->children;
}

void
scoped_render_container::end()
{
    if (traversal_)
    {
        *traversal_->next_ptr = nullptr;
        traversal_->next_ptr = &container_->next;
    }
}

}} // namespace alia::indie
