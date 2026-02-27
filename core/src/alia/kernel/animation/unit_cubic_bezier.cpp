#include <alia/kernel/animation/unit_cubic_bezier.h>

#include <cmath>

namespace alia {

namespace {

float
sample_curve_x(unit_cubic_bezier_coefficients const& coeff, float t)
{
    return ((coeff.ax * t + coeff.bx) * t + coeff.cx) * t;
}

float
sample_curve_y(unit_cubic_bezier_coefficients const& coeff, float t)
{
    return ((coeff.ay * t + coeff.by) * t + coeff.cy) * t;
}

float
sample_curve_derivative(unit_cubic_bezier_coefficients const& coeff, float t)
{
    return (3 * coeff.ax * t + 2 * coeff.bx) * t + coeff.cx;
}

} // namespace

unit_cubic_bezier_coefficients
compute_curve_coefficients(unit_cubic_bezier const& bezier)
{
    unit_cubic_bezier_coefficients coeff;
    coeff.cx = 3 * bezier.p1x;
    coeff.bx = 3 * (bezier.p2x - bezier.p1x) - coeff.cx;
    coeff.ax = 1 - coeff.cx - coeff.bx;
    coeff.cy = 3 * bezier.p1y;
    coeff.by = 3 * (bezier.p2y - bezier.p1y) - coeff.cy;
    coeff.ay = 1 - coeff.cy - coeff.by;
    return coeff;
}

float
solve_for_t_at_x_with_bisection_search(
    unit_cubic_bezier_coefficients const& coeff,
    float x,
    float error_tolerance)
{
    float lower = 0.0;
    float upper = 1.0;
    float t = x;
    while (true)
    {
        float x_at_t = sample_curve_x(coeff, t);
        if (std::fabs(x_at_t - x) < error_tolerance)
            return t;
        if (x > x_at_t)
            lower = t;
        else
            upper = t;
        t = (lower + upper) / 2;
    }
}

float
solve_for_t_at_x(
    unit_cubic_bezier_coefficients const& coeff,
    float x,
    float error_tolerance)
{
    // Newton's method should be faster, so try that first.
    float t = x;
    for (int i = 0; i != 8; ++i)
    {
        float x_error = sample_curve_x(coeff, t) - x;
        if (std::fabs(x_error) < error_tolerance)
            return t;
        float dx = sample_curve_derivative(coeff, t);
        if (std::fabs(dx) < 1e-6)
            break;
        t -= x_error / dx;
    }

    // If that fails, fallback to a bisection search.
    return solve_for_t_at_x_with_bisection_search(coeff, x, error_tolerance);
}

float
eval_curve_at_x(unit_cubic_bezier const& curve, float x, float epsilon)
{
    if (x <= 0)
        return 0;
    if (x >= 1)
        return 1;

    auto coeff = compute_curve_coefficients(curve);

    return sample_curve_y(coeff, solve_for_t_at_x(coeff, x, epsilon));
}

} // namespace alia
