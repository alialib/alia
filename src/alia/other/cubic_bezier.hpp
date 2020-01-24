#ifndef ALIA_OTHER_CUBIC_BEZIER_HPP
#define ALIA_OTHER_CUBIC_BEZIER_HPP

namespace alia {

// unit_cubic_bezier represents a cubic bezier whose end points are (0, 0)
// and (1, 1).
struct unit_cubic_bezier
{
    double p1x, p1y, p2x, p2y;
};

// unit_cubic_bezier_coefficients describes a unit cubic bezier curve by the
// coefficients in its parametric equation form.
struct unit_cubic_bezier_coefficients
{
    double ax, ay, bx, by, cx, cy;
};

unit_cubic_bezier_coefficients
compute_curve_coefficients(unit_cubic_bezier const& bezier);

// Solve for t at a point x in a unit cubic curve.
// This should only be called on curves that are expressible in y = f(x) form.
double
solve_for_t_at_x(
    unit_cubic_bezier_coefficients const& coeff,
    double x,
    double error_tolerance);

// This is the same as above but it always uses a bisection search.
// It's simple but likely slower and is primarily exposed for testing purposes.
double
solve_for_t_at_x_with_bisection_search(
    unit_cubic_bezier_coefficients const& coeff,
    double x,
    double error_tolerance);

// Evaluate a unit_cubic_bezier at the given x value.
// Since this is an approximation, the caller must specify a tolerance value
// to control the error in the result.
double
eval_curve_at_x(
    unit_cubic_bezier const& curve, double x, double error_tolerance);

} // namespace alia

#endif
