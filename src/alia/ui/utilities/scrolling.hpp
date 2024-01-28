#ifndef ALIA_UI_UTILITIES_SCROLLING_HPP
#define ALIA_UI_UTILITIES_SCROLLING_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events/input.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

std::optional<vector<2, double>>
detect_scroll(ui_event_context ctx, internal_element_ref element);

}

#endif
