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

rgb8 blend(rgb8 const& a, rgb8 const& b, float f)
{
    rgb8 r;
    r.r = uint8_t(a.r * f + b.r * (1 - f) + 0.5);
    r.g = uint8_t(a.g * f + b.g * (1 - f) + 0.5);
    r.b = uint8_t(a.b * f + b.b * (1 - f) + 0.5);
    return r;
}
rgb8 blend(rgb8 const& a, rgb8 const& b, double f)
{
    rgb8 r;
    r.r = uint8_t(a.r * f + b.r * (1 - f) + 0.5);
    r.g = uint8_t(a.g * f + b.g * (1 - f) + 0.5);
    r.b = uint8_t(a.b * f + b.b * (1 - f) + 0.5);
    return r;
}

rgba8 blend(rgba8 const& a, rgba8 const& b, float f)
{
    rgba8 r;
    r.r = uint8_t(a.r * f + b.r * (1 - f) + 0.5);
    r.g = uint8_t(a.g * f + b.g * (1 - f) + 0.5);
    r.b = uint8_t(a.b * f + b.b * (1 - f) + 0.5);
    r.a = uint8_t(a.a * f + b.a * (1 - f) + 0.5);
    return r;
}
rgba8 blend(rgba8 const& a, rgba8 const& b, double f)
{
    rgba8 r;
    r.r = uint8_t(a.r * f + b.r * (1 - f) + 0.5);
    r.g = uint8_t(a.g * f + b.g * (1 - f) + 0.5);
    r.b = uint8_t(a.b * f + b.b * (1 - f) + 0.5);
    r.a = uint8_t(a.a * f + b.a * (1 - f) + 0.5);
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
