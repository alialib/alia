#include <alia/color.hpp>
#include <iomanip>

namespace alia {

bool operator<(rgb8 const& a, rgb8 const& b)
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
bool operator<(rgba8 const& a, rgba8 const& b)
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
    if (a.b > b.b)
        return false;
    if (a.a < b.a)
        return true;
    return false;
}

rgb8 interpolate(rgb8 const& a, rgb8 const& b, double f)
{
    rgb8 r;
    r.r = uint8_t(a.r * (1 - f) + b.r * f + 0.5);
    r.g = uint8_t(a.g * (1 - f) + b.g * f + 0.5);
    r.b = uint8_t(a.b * (1 - f) + b.b * f + 0.5);
    return r;
}

rgba8 apply_alpha(rgb8 color, uint8_t alpha)
{
    return rgba8(
        multiply_uint8_channels(color.r, alpha),
        multiply_uint8_channels(color.g, alpha),
        multiply_uint8_channels(color.b, alpha),
        alpha);
}

rgba8 apply_alpha(rgba8 color, uint8_t alpha)
{
    return rgba8(
        multiply_uint8_channels(color.r, alpha),
        multiply_uint8_channels(color.g, alpha),
        multiply_uint8_channels(color.b, alpha),
        multiply_uint8_channels(color.a, alpha));
}

rgba8 interpolate(rgba8 const& a, rgba8 const& b, double f)
{
    rgba8 r;
    r.r = uint8_t(a.r * (1 - f) + b.r * f + 0.5);
    r.g = uint8_t(a.g * (1 - f) + b.g * f + 0.5);
    r.b = uint8_t(a.b * (1 - f) + b.b * f + 0.5);
    r.a = uint8_t(a.a * (1 - f) + b.a * f + 0.5);
    return r;
}

std::ostream& operator<<(std::ostream& s, rgb8 const& c)
{
    return s << "#" << std::hex << std::setw(2) << std::setfill('0') <<
        int(c.r) << int(c.g) << int(c.b);
}
std::ostream& operator<<(std::ostream& s, rgba8 const& c)
{
    return s << "#" << std::hex << std::setw(2) << std::setfill('0') <<
        int(c.r) << int(c.g) << int(c.b) << int(c.a);
}

}
