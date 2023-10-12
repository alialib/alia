#include <alia/indie/utilities/scrolling.hpp>

namespace alia { namespace indie {

std::optional<vector<2, double>>
detect_scroll(event_context ctx, widget const*)
{
    scroll_event* event;
    if (detect_event(ctx, &event))
        return event->delta;
    else
        return std::nullopt;
}

}} // namespace alia::indie
