#ifndef ALIA_UI_COLOR_HPP
#define ALIA_UI_COLOR_HPP

#include <alia/ui/common.hpp>

namespace alia {

template<class Channel>
struct channel_operations;

template<>
struct channel_operations<uint8_t>
{
    static constexpr uint8_t
    multiply(uint8_t a, uint8_t b) noexcept
    {
        return uint8_t(unsigned(a) * b / 0xff);
    }

    static constexpr uint8_t
    multiply(uint8_t a, double b) noexcept
    {
        return uint8_t(a * b + 0.5);
    }

    static constexpr uint8_t
    round(double value) noexcept
    {
        return uint8_t(value + 0.5);
    }

    static constexpr uint8_t
    max_value() noexcept
    {
        return 0xff;
    }

    static constexpr uint8_t
    from_normalized_double(double value) noexcept
    {
        return uint8_t(value * 255.0 + 0.5);
    }

    static constexpr double
    to_normalized_double(uint8_t value) noexcept
    {
        return value / 255.0;
    }

    static constexpr uint8_t
    from_normalized_float(float value) noexcept
    {
        return uint8_t(value * 255.0f + 0.5f);
    }

    static constexpr float
    to_normalized_float(uint8_t value) noexcept
    {
        return value / 255.0f;
    }
};

// RGB triplet
template<class Channel>
struct rgb
{
    Channel r, g, b;

    constexpr rgb() noexcept
    {
    }
    constexpr rgb(uint32_t hex) noexcept
        : r((hex >> 16) & 0xff), g((hex >> 8) & 0xff), b(hex & 0xff)
    {
    }
    constexpr rgb(Channel r, Channel g, Channel b) noexcept : r(r), g(g), b(b)
    {
    }

    constexpr auto
    operator<=>(rgb const&) const noexcept
        = default;
};
typedef rgb<uint8_t> rgb8;

std::ostream&
operator<<(std::ostream& s, rgb8 const& c);

template<class Channel>
constexpr rgb<Channel>
lerp(rgb<Channel> const& a, rgb<Channel> const& b, double t) noexcept
{
    rgb<Channel> r;
    r.r = channel_operations<Channel>::round(a.r + (b.r - a.r) * t);
    r.g = channel_operations<Channel>::round(a.g + (b.g - a.g) * t);
    r.b = channel_operations<Channel>::round(a.b + (b.b - a.b) * t);
    return r;
}

constexpr rgb8
hex_color(const char* hex)
{
    // Skip leading '#' if present.
    if (*hex == '#')
        ++hex;

    // Parse the hex digits.
    uint32_t value = 0;
    for (int i = 0; i < 6; ++i)
    {
        char c = hex[i];
        uint8_t digit;
        if (c >= '0' && c <= '9')
        {
            digit = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            digit = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            digit = c - 'A' + 10;
        }
        else // invalid hex digit
        {
            return rgb8(0, 0, 0);
        }
        value = (value << 4) | digit;
    }

    // Extract RGB components.
    uint8_t r = (value >> 16) & 0xff;
    uint8_t g = (value >> 8) & 0xff;
    uint8_t b = value & 0xff;

    return rgb8(r, g, b);
}

// RGBA w/ premultiplied alpha
template<class Channel>
struct rgba
{
    Channel r, g, b, a;

    constexpr rgba() noexcept
    {
    }
    constexpr rgba(Channel r, Channel g, Channel b, Channel a) noexcept
        : r(r), g(g), b(b), a(a)
    {
    }
    constexpr rgba(rgb<Channel> color) noexcept
        : r(color.r),
          g(color.g),
          b(color.b),
          a(channel_operations<Channel>::max_value())
    {
    }
    constexpr rgba(rgb<Channel> color, Channel a) noexcept
        : r(color.r), g(color.g), b(color.b), a(a)
    {
    }

    constexpr auto
    operator<=>(rgba const&) const noexcept
        = default;
};
typedef rgba<uint8_t> rgba8;

std::ostream&
operator<<(std::ostream& s, rgba8 const& c);

// premultiply the color by the alpha to form an rgba8 value
template<class Channel, class Alpha>
constexpr rgba<Channel>
apply_alpha(rgb<Channel> color, Alpha alpha) noexcept
{
    return rgba<Channel>(
        channel_operations<Channel>::multiply(color.r, alpha),
        channel_operations<Channel>::multiply(color.g, alpha),
        channel_operations<Channel>::multiply(color.b, alpha),
        alpha);
}
// same, but with an alpha channel already present
template<class Channel, class Alpha>
constexpr rgba<Channel>
apply_alpha(rgba<Channel> color, Alpha alpha) noexcept
{
    return rgba<Channel>(
        channel_operations<Channel>::multiply(color.r, alpha),
        channel_operations<Channel>::multiply(color.g, alpha),
        channel_operations<Channel>::multiply(color.b, alpha),
        channel_operations<Channel>::multiply(color.a, alpha));
}

template<class Channel>
constexpr rgba<Channel>
lerp(rgba<Channel> const& a, rgba<Channel> const& b, double t) noexcept
{
    rgba<Channel> r;
    r.r = channel_operations<Channel>::round(a.r + (b.r - a.r) * t);
    r.g = channel_operations<Channel>::round(a.g + (b.g - a.g) * t);
    r.b = channel_operations<Channel>::round(a.b + (b.b - a.b) * t);
    r.a = channel_operations<Channel>::round(a.a + (b.a - a.a) * t);
    return r;
}

// HSL color value

struct hsl
{
    float h, s, l;

    constexpr auto
    operator<=>(hsl const&) const noexcept
        = default;
};

rgb8
to_rgb8(hsl const& hsl) noexcept;

hsl
to_hsl(rgb8 const& rgb) noexcept;

} // namespace alia

#endif
