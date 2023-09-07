#ifndef ALIA_INDIE_UTILITIES_HIT_TESTING_HPP
#define ALIA_INDIE_UTILITIES_HIT_TESTING_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

enum class hit_test_type
{
    // hit testing for potential mouse interactions
    MOUSE,
    // hit testing for potential scroll wheel movement
    WHEEL
};

struct hit_test_base
{
    // the type of test
    hit_test_type type;
};

struct mouse_hit_test_result
{
    external_widget_handle widget;
    mouse_cursor cursor;
    layout_box hit_box;
    std::string tooltip_message;
};

struct mouse_hit_test : hit_test_base
{
    std::optional<mouse_hit_test_result> result;

    mouse_hit_test() : hit_test_base{hit_test_type::MOUSE}
    {
    }
};

struct wheel_hit_test : hit_test_base
{
    std::optional<external_widget_handle> result;

    wheel_hit_test() : hit_test_base{hit_test_type::WHEEL}
    {
    }
};

}} // namespace alia::indie

#endif
