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

inline bool
operator==(rgb8 const& a, rgb8 const& b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
inline bool
operator!=(rgb8 const& a, rgb8 const& b)
{
    return !(a == b);
}
bool
operator<(rgb8 const& a, rgb8 const& b);

std::ostream&
operator<<(std::ostream& s, rgb8 const& c);

// lerp(a, b, t) = a * (1 - t) + b * t
rgb8
lerp(rgb8 const& a, rgb8 const& b, double t);

} // namespace alia

#endif
