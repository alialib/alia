#include <alia/ui/utilities/scrolling.hpp>

namespace alia {

std::optional<vector<2, double>>
detect_scroll(ui_event_context ctx, internal_element_ref)
{
    scroll_event* event;
    if (detect_event(ctx, &event))
        return event->delta;
    else
        return std::nullopt;
}

} // namespace alia
