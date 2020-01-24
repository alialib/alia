#ifndef ALIA_OTHER_CUBIC_BEZIER_HPP
#define ALIA_OTHER_CUBIC_BEZIER_HPP

namespace alia {

// unit_cubic_bezier represents a cubic bezier whose end points are (0, 0)
// and (1, 1).
struct unit_cubic_bezier
{
    double p1x, p1y, p2x, p2y;
};

// Evaluate a unit_cubic_bezier at the given x value.
// Since this is an approximation, the caller must specify a tolerance value
// to control the error in the result.
double
eval_curve_at_x(
    unit_cubic_bezier const& curve, double x, double error_tolerance);

} // namespace alia

#endif
