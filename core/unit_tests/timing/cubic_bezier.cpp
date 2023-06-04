#include <alia/core/timing/cubic_bezier.hpp>

#include <testing.hpp>

using namespace alia;

TEST_CASE("linear cubic bezier", "[timing][cubic_bezier]")
{
    double error_tolerance = 0.00001;

    unit_cubic_bezier curve = {0.1, 0.1, 0.9, 0.9};
    auto coeff = compute_curve_coefficients(curve);
    for (int i = 0; i <= 10; ++i)
    {
        REQUIRE(
            solve_for_t_at_x(coeff, 0.1 * i, error_tolerance)
            == Approx(solve_for_t_at_x_with_bisection_search(
                          coeff, 0.1 * i, error_tolerance))
                   .margin(error_tolerance * 2));
        REQUIRE(
            eval_curve_at_x(curve, 0.1 * i, error_tolerance)
            == Approx(0.1 * i).margin(error_tolerance));
    }
}

TEST_CASE("nonlinear cubic bezier", "[timing][cubic_bezier]")
{
    double error_tolerance = 0.0001;

    unit_cubic_bezier curve = {0.25, 0.1, 0.25, 1};
    double test_values[11][2]
        = {{1.0000, 1.0000},
           {0.7965, 0.9747},
           {0.6320, 0.9056},
           {0.5005, 0.8029},
           {0.3960, 0.6768},
           {0.3125, 0.5375},
           {0.2440, 0.3952},
           {0.1845, 0.2601},
           {0.1280, 0.1424},
           {0.0685, 0.0523},
           {0.0000, 0.0000}};
    auto coeff = compute_curve_coefficients(curve);
    for (int i = 0; i != 11; ++i)
    {
        REQUIRE(
            solve_for_t_at_x(coeff, 0.1 * i, error_tolerance)
            == Approx(solve_for_t_at_x_with_bisection_search(
                          coeff, 0.1 * i, error_tolerance))
                   .margin(error_tolerance * 2));
        REQUIRE(
            eval_curve_at_x(curve, test_values[i][0], error_tolerance)
            == Approx(test_values[i][1]).margin(error_tolerance));
    }
}

TEST_CASE("degenerate cubic bezier", "[timing][cubic_bezier]")
{
    double error_tolerance = 0.0000001;

    unit_cubic_bezier curve = {0, 0.5, 0, 1};
    auto coeff = compute_curve_coefficients(curve);

    for (int i = 0; i != 11; ++i)
    {
        REQUIRE(
            solve_for_t_at_x(coeff, 0.1 * i, error_tolerance)
            == Approx(solve_for_t_at_x_with_bisection_search(
                          coeff, 0.1 * i, error_tolerance))
                   .margin(error_tolerance * 2));
    }

    REQUIRE(
        solve_for_t_at_x(coeff, 0.00001, error_tolerance)
        == Approx(solve_for_t_at_x_with_bisection_search(
                      coeff, 0.00001, error_tolerance))
               .margin(error_tolerance * 2));
}
