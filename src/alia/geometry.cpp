#include <alia/geometry.hpp>

namespace alia {

struct unit_cubic_bezier_coefficients
{
    double ax, ay, bx, by, cx, cy;
};

static void
compute_curve_coefficients(
    unit_cubic_bezier_coefficients* coeff, unit_cubic_bezier const& bezier)
{
    coeff->cx = 3 * bezier.p1[0];
    coeff->bx = 3 * (bezier.p2[0] - bezier.p1[0]) - coeff->cx;
    coeff->ax = 1 - coeff->cx - coeff->bx;
    coeff->cy = 3 * bezier.p1[1];
    coeff->by = 3 * (bezier.p2[1] - bezier.p1[1]) - coeff->cy;
    coeff->ay = 1 - coeff->cy - coeff->by;
}

static double
sample_curve_x(unit_cubic_bezier_coefficients const& coeff, double t)
{
    return ((coeff.ax * t + coeff.bx) * t + coeff.cx) * t;
}

static double
sample_curve_y(unit_cubic_bezier_coefficients const& coeff, double t)
{
    return ((coeff.ay * t + coeff.by) * t + coeff.cy) * t;
}

static double
sample_curve_derivative(unit_cubic_bezier_coefficients const& coeff, double t)
{
    return (3 * coeff.ax * t + 2 * coeff.bx) *t + coeff.cx;
}

static double
solve_for_t_at_x(
    unit_cubic_bezier_coefficients const& coeff, double x, double epsilon)
{
    // Newton's method should be faster, so try that first.
    double t = x;
    for (int i = 0; i != 8; ++i)
    {
        double x_error = sample_curve_x(coeff, t) - x;
        if (std::fabs(x_error) < epsilon)
            return t;
        double dx = sample_curve_derivative(coeff, t);
        if (std::fabs(dx) < 1e-6)
            break;
        t -= x_error / dx;
    }

    // If that fails, fallback to a bisection search.
    double lower = 0.0;
    double upper = 1.0;
    t = x;
    while (lower < upper)
    {
        double x_at_t = sample_curve_x(coeff, t);
        if (std::fabs(x_at_t - x) < epsilon)
            return t;
        if (x > x_at_t)
            lower = t;
        else
            upper = t;
        t = (lower + upper) / 2;
    }
    return t;
}

double
eval_curve_at_x(unit_cubic_bezier const& curve, double x, double epsilon)
{
    if (x <= 0)
        return 0;
    if (x >= 1)
        return 1;

    unit_cubic_bezier_coefficients coeff;
    compute_curve_coefficients(&coeff, curve);

    return sample_curve_y(coeff, solve_for_t_at_x(coeff, x, epsilon));
}

}
