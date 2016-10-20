#include <alia/geometry.hpp>

#define BOOST_TEST_MODULE id
#include "test.hpp"

BOOST_AUTO_TEST_CASE(id_test)
{
    using namespace alia;

    double epsilon = 0.00001;
    double tolerance = 0.00011;

    unit_cubic_bezier linear(
        make_vector<double>(0, 0),
        make_vector<double>(1, 1));
    for (int i = 0; i <= 10; ++i)
    {
        BOOST_CHECK_SMALL(
            eval_curve_at_x(linear, 0.1 * i, epsilon) - 0.1 * i,
            tolerance);
    }

    unit_cubic_bezier nonlinear(
        make_vector<double>(0.25, 0.1),
        make_vector<double>(0.25, 1));
    float test_values[11][2] = {
        { 1.0000f, 1.0000f },
        { 0.7965f, 0.9747f },
        { 0.6320f, 0.9056f },
        { 0.5005f, 0.8029f },
        { 0.3960f, 0.6768f },
        { 0.3125f, 0.5375f },
        { 0.2440f, 0.3952f },
        { 0.1845f, 0.2601f },
        { 0.1280f, 0.1424f },
        { 0.0685f, 0.0523f },
        { 0.0000f, 0.0000f } };
    for (int i = 0; i != 11; ++i)
    {
        BOOST_CHECK_SMALL(
            eval_curve_at_x(nonlinear, test_values[i][0], epsilon) -
                test_values[i][1],
            tolerance);
    }
}
