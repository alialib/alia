#pragma once

#include <alia/abi/color.h>

namespace alia {

using color = alia_rgba;

using srgb8 = alia_srgb8;
using rgb = alia_rgb;
using oklab = alia_oklab;
using oklch = alia_oklch;

constexpr srgb8
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
            return srgb8{0, 0, 0};
        }
        value = (value << 4) | digit;
    }

    // Extract RGB components.
    uint8_t r = (value >> 16) & 0xff;
    uint8_t g = (value >> 8) & 0xff;
    uint8_t b = value & 0xff;

    return srgb8{r, g, b};
}

constexpr color RED = {1.f, 0.f, 0.f, 1.f};
constexpr color BLUE = {0.f, 0.f, 1.f, 1.f};
constexpr color GRAY = {0.5f, 0.5f, 0.5f, 1.f};

} // namespace alia
