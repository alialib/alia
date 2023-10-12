#ifndef ALIA_INDIE_UTILITIES_SCROLLING_HPP
#define ALIA_INDIE_UTILITIES_SCROLLING_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>

namespace alia { namespace indie {

std::optional<vector<2, double>>
detect_scroll(event_context ctx, widget const* widget);

}} // namespace alia::indie

#endif
