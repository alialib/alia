#include <alia/color.hpp>

namespace alia {

rgb8 blend(rgb8 const& a, rgb8 const& b, float f)
{
    rgb8 r;
    r.r = uint8(a.r * f + b.r * (1 - f) + 0.5);
    r.g = uint8(a.g * f + b.g * (1 - f) + 0.5);
    r.b = uint8(a.b * f + b.b * (1 - f) + 0.5);
    return r;
}
rgb8 scale(rgb8 const& c, float f)
{
    rgb8 r;
    r.r = uint8(c.r * f + 0.5);
    r.g = uint8(c.g * f + 0.5);
    r.b = uint8(c.b * f + 0.5);
    return r;
}

rgba8 blend(rgba8 const& a, rgba8 const& b, float f)
{
    rgba8 r;
    r.r = uint8(a.r * f + b.r * (1 - f) + 0.5);
    r.g = uint8(a.g * f + b.g * (1 - f) + 0.5);
    r.b = uint8(a.b * f + b.b * (1 - f) + 0.5);
    r.a = uint8(a.a * f + b.a * (1 - f) + 0.5);
    return r;
}
rgba8 scale(rgba8 const& c, float f)
{
    rgba8 r;
    r.r = uint8(c.r * f + 0.5);
    r.g = uint8(c.g * f + 0.5);
    r.b = uint8(c.b * f + 0.5);
    r.a = uint8(c.a * f + 0.5);
    return r;
}

}
