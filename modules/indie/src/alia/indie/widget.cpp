#include <alia/indie/system/object.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

void
add_widget(widget_traversal& traversal, widget* node)
{
    *traversal.next_ptr = node;
    traversal.next_ptr = &node->next;
}

void
render_children(SkCanvas& canvas, widget_container& container)
{
    for (widget* node = container.children; node; node = node->next)
        node->render(canvas);
}

void
scoped_widget_container::begin(
    widget_traversal& traversal, widget_container* container)
{
    traversal_ = &traversal;
    container_ = container;

    *traversal.next_ptr = container;
    traversal.next_ptr = &container->children;
}

void
scoped_widget_container::end()
{
    if (traversal_)
    {
        *traversal_->next_ptr = nullptr;
        traversal_->next_ptr = &container_->next;
    }
}

}} // namespace alia::indie
