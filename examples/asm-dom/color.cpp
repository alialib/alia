#include "color.hpp"
#include <iomanip>

namespace alia {

bool
operator<(rgb8 const& a, rgb8 const& b)
{
    if (a.r < b.r)
        return true;
    if (a.r > b.r)
        return false;
    if (a.g < b.g)
        return true;
    if (a.g > b.g)
        return false;
    if (a.b < b.b)
        return true;
    return false;
}

rgb8
interpolate(rgb8 const& a, rgb8 const& b, double f)
{
    rgb8 r;
    r.r = uint8_t(a.r * (1 - f) + b.r * f + 0.5);
    r.g = uint8_t(a.g * (1 - f) + b.g * f + 0.5);
    r.b = uint8_t(a.b * (1 - f) + b.b * f + 0.5);
    return r;
}

} // namespace alia