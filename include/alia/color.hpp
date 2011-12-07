#ifndef ALIA_COLOR_HPP
#define ALIA_COLOR_HPP

#include <alia/typedefs.hpp>
#include <cradle/reflection_interface.hpp> // TODO

namespace alia {

// RGB
struct rgb8
{
    rgb8() {}
    rgb8(uint8 r, uint8 g, uint8 b) : r(r), g(g), b(b) {}
    uint8 r, g, b;
};
CRADLE_REFLECTION(rgb8, (r)(g)(b))
// comparison
static inline bool operator==(rgb8 const& a, rgb8 const& b)
{ return a.r == b.r && a.g == b.g && a.b == b.b; }
static inline bool operator!=(rgb8 const& a, rgb8 const& b)
{ return !(a == b); }
// blend(a, b, factor) = a * factor + b * (1 - factor)
rgb8 blend(rgb8 const& a, rgb8 const& b, float factor);
rgb8 blend(rgb8 const& a, rgb8 const& b, double factor);
// scale(c, factor) = c * factor
rgb8 scale(rgb8 const& c, float factor);
rgb8 scale(rgb8 const& c, double factor);

// RGBA
struct rgba8
{
    rgba8() {}
    rgba8(uint8 r, uint8 g, uint8 b, uint8 a) : r(r), g(g), b(b), a(a) {}
    rgba8(rgb8 const& c, uint8 a = 0xff) : r(c.r), g(c.g), b(c.b), a(a) {}
    uint8 r, g, b, a;
};
CRADLE_REFLECTION(rgba8, (r)(g)(b)(a))
// comparison
static inline bool operator==(rgba8 const& a, rgba8 const& b)
{ return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
static inline bool operator!=(rgba8 const& a, rgba8 const& b)
{ return !(a == b); }
// blend(a, b, factor) = a * factor + b * (1 - factor)
rgba8 blend(rgba8 const& a, rgba8 const& b, float factor);
rgba8 blend(rgba8 const& a, rgba8 const& b, double factor);
// scale(c, factor) = c * factor
rgba8 scale(rgba8 const& c, float factor);
rgba8 scale(rgba8 const& c, double factor);

}

#endif
