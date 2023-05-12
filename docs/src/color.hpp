#ifndef ALIA_COLOR_HPP
#define ALIA_COLOR_HPP

#include <cstdint>
#include <iostream>

namespace alia {

// RGB color/pixel
template<class Channel>
struct rgb
{
    rgb()
    {
    }
    rgb(Channel r, Channel g, Channel b) : r(r), g(g), b(b)
    {
    }
    Channel r, g, b;
};
typedef rgb<uint8_t> rgb8;

static inline bool
operator==(rgb8 const& a, rgb8 const& b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
static inline bool
operator!=(rgb8 const& a, rgb8 const& b)
{
    return !(a == b);
}
bool
operator<(rgb8 const& a, rgb8 const& b);

std::ostream&
operator<<(std::ostream& s, rgb8 const& c);

// interpolate(a, b, factor) = a * (1 - factor) + b * factor
rgb8
interpolate(rgb8 const& a, rgb8 const& b, double factor);

} // namespace alia

#endif
