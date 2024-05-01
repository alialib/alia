#ifndef ALIA_UI_UTILITIES_HIT_TESTING_HPP
#define ALIA_UI_UTILITIES_HIT_TESTING_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/system/input_constants.hpp>

namespace alia {

// Perform mouse hit testing on a box.
void
hit_test_box(
    hit_test_base& test,
    vector<2, double> const& point,
    internal_element_id element,
    layout_box const& box,
    mouse_cursor cursor = mouse_cursor::DEFAULT);

} // namespace alia

#endif
