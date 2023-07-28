#ifndef ALIA_INDIE_UTILITIES_HIT_TESTING_HPP
#define ALIA_INDIE_UTILITIES_HIT_TESTING_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>

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
    // the point we're testing
    vector<2, double> point;
};

struct mouse_hit_test_result
{
    external_component_id id;
    mouse_cursor cursor;
    layout_box hit_box;
    std::string tooltip_message;
};

struct mouse_hit_test : hit_test_base
{
    std::optional<mouse_hit_test_result> result;

    mouse_hit_test(vector<2, double> point)
        : hit_test_base{hit_test_type::MOUSE, point}
    {
    }
};

struct wheel_hit_test : hit_test_base
{
    std::optional<external_component_id> result;

    wheel_hit_test(vector<2, double> point)
        : hit_test_base{hit_test_type::WHEEL, point}
    {
    }
};

}} // namespace alia::indie

#endif
