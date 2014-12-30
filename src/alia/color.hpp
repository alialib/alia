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

}

namespace std
{
    template<class Channel>
    struct hash<alia::rgb<Channel> >
    {
        size_t operator()(alia::rgb<Channel> const& x) const
        {
            return
                alia::combine_hashes(
                    alia::combine_hashes(
                        hash<Channel>()(x.r),
                        hash<Channel>()(x.g)),
                    hash<Channel>()(x.b));
        }
    };
}

namespace alia {

static inline uint8_t multiply_uint8_channels(uint8_t a, uint8_t b)
{ return uint8_t(unsigned(a) * b / 0xff); }

// interpolate(a, b, factor) = a * (1 - factor) + b * factor
rgb8 interpolate(rgb8 const& a, rgb8 const& b, double factor);

// standard color names
static rgb8 const
    white   (0xff, 0xff, 0xff),
    silver  (0xc0, 0xc0, 0xc0),
    gray    (0x80, 0x80, 0x80),
    black   (0x00, 0x00, 0x00),
    red     (0xff, 0x00, 0x00),
    maroon  (0x80, 0x00, 0x00),
    yellow  (0xff, 0xff, 0x00),
    olive   (0x80, 0x80, 0x00),
    lime    (0x00, 0xff, 0x00),
    green   (0x00, 0x80, 0x00),
    aqua    (0x00, 0xff, 0xff),
    teal    (0x00, 0x80, 0x80),
    blue    (0x00, 0x00, 0xff),
    navy    (0x00, 0x00, 0x80),
    fuchsia (0xff, 0x00, 0xff),
    purple  (0x80, 0x00, 0x80);

static inline uint8_t max_channel_value(uint8_t) { return 0xff; }

// RGBA color/pixel, w/ premultiplied alpha
template<class Channel>
struct rgba
{
    rgba() {}
    rgba(Channel r, Channel g, Channel b, Channel a)
      : r(r), g(g), b(b), a(a)
    {}
    rgba(rgb<Channel> color)
      : r(color.r), g(color.g), b(color.b), a(max_channel_value(Channel()))
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

}

namespace std
{
    template<class Channel>
    struct hash<alia::rgba<Channel> >
    {
        size_t operator()(alia::rgba<Channel> const& x) const
        {
            return
                alia::combine_hashes(
                    alia::combine_hashes(
                        hash<Channel>()(x.r),
                        hash<Channel>()(x.g)),
                    alia::combine_hashes(
                        hash<Channel>()(x.b),
                        hash<Channel>()(x.a)));
        }
    };
}

namespace alia {

// premultiply the color by the alpha to form an rgba8 value
rgba8 apply_alpha(rgb8 color, uint8_t alpha);
rgba8 apply_alpha(rgba8 color, uint8_t alpha);

// interpolate(a, b, factor) = a * (1 - factor) + b * factor
rgba8 interpolate(rgba8 const& a, rgba8 const& b, double factor);

}

#endif
