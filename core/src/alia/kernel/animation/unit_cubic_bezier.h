#pragma once

#include <alia/abi/kernel/animation.h>

namespace alia {

using unit_cubic_bezier = alia_unit_cubic_bezier;

// unit_cubic_bezier_coefficients describes a unit cubic bezier curve by the
// coefficients in its parametric equation form.
struct unit_cubic_bezier_coefficients
{
    float ax, ay, bx, by, cx, cy;
};

unit_cubic_bezier_coefficients
compute_curve_coefficients(unit_cubic_bezier const& bezier);

// Solve for t at a point x in a unit cubic curve.
// This should only be called on curves that are expressible in y = f(x) form.
float
solve_for_t_at_x(
    unit_cubic_bezier_coefficients const& coeff,
    float x,
    float error_tolerance);

// This is the same as above but always uses a bisection search.
// It's simple but likely slower and is primarily exposed for testing purposes.
float
solve_for_t_at_x_with_bisection_search(
    unit_cubic_bezier_coefficients const& coeff,
    float x,
    float error_tolerance);

// Evaluate a unit_cubic_bezier at the given x value.
// Since this is an approximation, the caller must specify a tolerance value
// to control the error in the result.
float
eval_curve_at_x(
    unit_cubic_bezier const& curve, float x, float error_tolerance);

} // namespace alia
