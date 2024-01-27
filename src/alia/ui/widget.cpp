#include <alia/indie/system/object.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

void
render_children(render_event& event, widget_container& container)
{
    for (widget* node = container.children; node; node = node->next)
        node->render(event);
}

}} // namespace alia::indie
