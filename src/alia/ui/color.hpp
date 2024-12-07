#ifndef ALIA_UI_COLOR_HPP
#define ALIA_UI_COLOR_HPP

#include <alia/ui/common.hpp>

namespace alia {

// RGB color/pixel
template<class Channel>
struct rgb
{
    constexpr rgb()
    {
    }
    constexpr rgb(Channel r, Channel g, Channel b) : r(r), g(g), b(b)
    {
    }
    Channel r, g, b;

    auto
    operator<=>(rgb const&) const
        = default;
};
typedef rgb<uint8_t> rgb8;

std::ostream&
operator<<(std::ostream& s, rgb8 const& c);

} // namespace alia

template<class Channel>
struct std::hash<alia::rgb<Channel>>
{
    size_t
    operator()(alia::rgb<Channel> const& x) const
    {
        return alia::combine_hashes(
            alia::combine_hashes(hash<Channel>()(x.r), hash<Channel>()(x.g)),
            hash<Channel>()(x.b));
    }
};

namespace alia {

inline uint8_t
multiply_uint8_channels(uint8_t a, uint8_t b)
{
    return uint8_t(unsigned(a) * b / 0xff);
}

// interpolate(a, b, factor) = a * (1 - factor) + b * factor
rgb8
interpolate(rgb8 const& a, rgb8 const& b, double factor);

// standard color names
rgb8 const white(0xff, 0xff, 0xff), silver(0xc0, 0xc0, 0xc0),
    gray(0x80, 0x80, 0x80), black(0x00, 0x00, 0x00), red(0xff, 0x00, 0x00),
    maroon(0x80, 0x00, 0x00), yellow(0xff, 0xff, 0x00),
    olive(0x80, 0x80, 0x00), lime(0x00, 0xff, 0x00), green(0x00, 0x80, 0x00),
    aqua(0x00, 0xff, 0xff), teal(0x00, 0x80, 0x80), blue(0x00, 0x00, 0xff),
    navy(0x00, 0x00, 0x80), fuchsia(0xff, 0x00, 0xff),
    purple(0x80, 0x00, 0x80);

constexpr uint8_t
max_channel_value(uint8_t)
{
    return 0xff;
}

// RGBA color/pixel, w/ premultiplied alpha
template<class Channel>
struct rgba
{
    constexpr rgba()
    {
    }
    constexpr rgba(Channel r, Channel g, Channel b, Channel a)
        : r(r), g(g), b(b), a(a)
    {
    }
    constexpr rgba(rgb<Channel> color)
        : r(color.r), g(color.g), b(color.b), a(max_channel_value(Channel()))
    {
    }
    constexpr rgba(rgb<Channel> color, Channel a)
        : r(color.r), g(color.g), b(color.b), a(a)
    {
    }
    Channel r, g, b, a;

    auto
    operator<=>(rgba const&) const
        = default;
};
typedef rgba<uint8_t> rgba8;

std::ostream&
operator<<(std::ostream& s, rgba8 const& c);

} // namespace alia

template<class Channel>
struct std::hash<alia::rgba<Channel>>
{
    size_t
    operator()(alia::rgba<Channel> const& x) const
    {
        return alia::combine_hashes(
            alia::combine_hashes(hash<Channel>()(x.r), hash<Channel>()(x.g)),
            alia::combine_hashes(hash<Channel>()(x.b), hash<Channel>()(x.a)));
    }
};

namespace alia {

// premultiply the color by the alpha to form an rgba8 value
rgba8
apply_alpha(rgb8 color, uint8_t alpha);
rgba8
apply_alpha(rgba8 color, uint8_t alpha);

// interpolate(a, b, factor) = a * (1 - factor) + b * factor
rgba8
interpolate(rgba8 const& a, rgba8 const& b, double factor);

} // namespace alia

#endif
