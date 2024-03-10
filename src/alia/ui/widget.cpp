#include <alia/ui/system/object.hpp>
#include <alia/ui/widget.hpp>

namespace alia {

void
render_children(render_event& event, widget_container& container)
{
    for (widget* node = container.children; node; node = node->next)
        node->render(event);
}

} // namespace alia
