#include <alia/ui/color.hpp>
#include <iomanip>

namespace alia {

std::ostream&
operator<<(std::ostream& s, rgb8 const& c)
{
    return s << "#" << std::hex << std::setw(2) << std::setfill('0')
             << int(c.r) << int(c.g) << int(c.b);
}

std::ostream&
operator<<(std::ostream& s, rgba8 const& c)
{
    return s << "#" << std::hex << std::setw(2) << std::setfill('0')
             << int(c.r) << int(c.g) << int(c.b) << int(c.a);
}

rgb8
to_rgb8(hsl const& hsl) noexcept
{
    float const h = hsl.h / 60.0f;
    float const c = (1 - std::abs(2 * hsl.l - 1)) * hsl.s;
    float const x = c * (1 - std::abs(fmod(h, 2.0f) - 1));
    float const m = hsl.l - c / 2.0f;

    float r, g, b;
    switch (static_cast<int>(h))
    {
        case 0:
            r = c;
            g = x;
            b = 0;
            break;
        case 1:
            r = x;
            g = c;
            b = 0;
            break;
        case 2:
            r = 0;
            g = c;
            b = x;
            break;
        case 3:
            r = 0;
            g = x;
            b = c;
            break;
        case 4:
            r = x;
            g = 0;
            b = c;
            break;
        case 5:
            r = c;
            g = 0;
            b = x;
            break;
    }

    return rgb8(
        channel_operations<uint8_t>::from_normalized_float(r + m),
        channel_operations<uint8_t>::from_normalized_float(g + m),
        channel_operations<uint8_t>::from_normalized_float(b + m));
}

hsl
to_hsl(rgb8 const& rgb) noexcept
{
    float const r = channel_operations<uint8_t>::to_normalized_float(rgb.r);
    float const g = channel_operations<uint8_t>::to_normalized_float(rgb.g);
    float const b = channel_operations<uint8_t>::to_normalized_float(rgb.b);

    float const max_val = std::max({r, g, b});
    float const min_val = std::min({r, g, b});

    // Calculate lightness.
    float const l = (max_val + min_val) / 2.0f;

    // If all RGB values are equal, it's a shade of gray.
    if (max_val == min_val)
    {
        return hsl(0.0f, 0.0f, l);
    }

    // Calculate saturation.
    float const delta = max_val - min_val;
    float const s = l > 0.5f ? delta / (2.0f - max_val - min_val)
                             : delta / (max_val + min_val);

    // Calculate hue.
    float h;
    if (max_val == r)
    {
        h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
    }
    else if (max_val == g)
    {
        h = (b - r) / delta + 2.0f;
    }
    else
    {
        h = (r - g) / delta + 4.0f;
    }
    h *= 60.0f;

    return hsl(h, s, l);
}

} // namespace alia
