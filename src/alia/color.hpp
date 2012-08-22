#ifndef ALIA_COLOR_HPP
#define ALIA_COLOR_HPP

#include <alia/common.hpp>

namespace alia {

// RGB color/pixel
template<class Channel>
struct rgb
{
    rgb() {}
    rgb(Channel r, Channel g, Channel b)
      : r(r), g(g), b(b)
    {}
    Channel r, g, b;
};
typedef rgb<uint8_t> rgb8;

static inline bool operator==(rgb8 const& a, rgb8 const& b)
{ return a.r == b.r && a.g == b.g && a.b == b.b; }
static inline bool operator!=(rgb8 const& a, rgb8 const& b)
{ return !(a == b); }
bool operator<(rgb8 const& a, rgb8 const& b);

std::ostream& operator<<(std::ostream& s, rgb8 const& c);

static inline uint8_t multiply_uint8_channels(uint8_t a, uint8_t b)
{ return uint8_t(unsigned(a) * b / 0xff); }

// blend(a, b, factor) = a * factor + b * (1 - factor)
rgb8 blend(rgb8 const& a, rgb8 const& b, float factor);
rgb8 blend(rgb8 const& a, rgb8 const& b, double factor);

// RGBA color/pixel, w/ premultiplied alpha
template<class Channel>
struct rgba
{
    rgba() {}
    rgba(Channel r, Channel g, Channel b, Channel a)
      : r(r), g(g), b(b), a(a)
    {}
    Channel r, g, b, a;
};
typedef rgba<uint8_t> rgba8;

static inline bool operator==(rgba8 const& a, rgba8 const& b)
{ return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
static inline bool operator!=(rgba8 const& a, rgba8 const& b)
{ return !(a == b); }
bool operator<(rgba8 const& a, rgba8 const& b);

std::ostream& operator<<(std::ostream& s, rgba8 const& c);

// premultiply the color by the alpha to form an rgba8 value
rgba8 apply_alpha(rgb8 color, uint8_t alpha);

// blend(a, b, factor) = a * factor + b * (1 - factor)
rgba8 blend(rgba8 const& a, rgba8 const& b, float factor);
rgba8 blend(rgba8 const& a, rgba8 const& b, double factor);

}

#endif
