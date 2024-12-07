#ifndef ALIA_UI_THEME_HPP
#define ALIA_UI_THEME_HPP

#include <alia/ui/color.hpp>

namespace alia {

// prototype based on Material Design 3 color roles
struct theme_colors
{
    // primary
    rgb8 primary;
    rgb8 on_primary;
    rgb8 primary_container;
    rgb8 on_primary_container;

    // secondary
    rgb8 secondary;
    rgb8 on_secondary;
    rgb8 secondary_container;
    rgb8 on_secondary_container;

    // tertiary
    rgb8 tertiary;
    rgb8 on_tertiary;
    rgb8 tertiary_container;
    rgb8 on_tertiary_container;

    // error
    rgb8 error;
    rgb8 on_error;
    rgb8 error_container;
    rgb8 on_error_container;

    // surfaces
    rgb8 surface_dim;
    rgb8 surface;
    rgb8 surface_bright;

    // surface containers, lowest to highest
    rgb8 surface_container_levels[5];

    // on surfaces
    rgb8 on_surface;
    rgb8 on_surface_variant;

    // outline
    rgb8 outline;
    rgb8 outline_variant;

    // inverse colors
    rgb8 inverse_surface;
    rgb8 inverse_on_surface;
    rgb8 inverse_primary;

    // shadow
    rgb8 shadow;
    rgb8 scrim;
};

constexpr rgb8
hex_color(const char* hex)
{
    uint32_t value = 0;

    // Skip leading '#' if present
    if (hex[0] == '#')
    {
        hex++;
    }

    // Parse each hex digit
    for (int i = 0; i < 6; i++)
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
        else
        {
            // invalid hex digit
            return rgb8(0, 0, 0);
        }
        value = (value << 4) | digit;
    }

    // Extract RGB components
    uint8_t r = (value >> 16) & 0xFF;
    uint8_t g = (value >> 8) & 0xFF;
    uint8_t b = value & 0xFF;

    return rgb8(r, g, b);
}

static theme_colors default_light_theme = {
    .primary = hex_color("#515b92"),
    .on_primary = hex_color("#ffffff"),
    .primary_container = hex_color("#dee0ff"),
    .on_primary_container = hex_color("#0a154b"),
    .secondary = hex_color("#5b5d72"),
    .on_secondary = hex_color("#ffffff"),
    .secondary_container = hex_color("#dfe1f9"),
    .on_secondary_container = hex_color("#171a2c"),
    .tertiary = hex_color("#76546d"),
    .on_tertiary = hex_color("#ffffff"),
    .tertiary_container = hex_color("#ffd7f2"),
    .on_tertiary_container = hex_color("#2d1228"),
    .error = hex_color("#ba1a1a"),
    .on_error = hex_color("#ffffff"),
    .error_container = hex_color("#ffdad6"),
    .on_error_container = hex_color("#410002"),
    .surface_dim = hex_color("#dbd9e0"),
    .surface = hex_color("#fbf8ff"),
    .surface_bright = hex_color("#fbf8ff"),
    .surface_container_levels = {
        hex_color("#ffffff"),
        hex_color("#f5f2fa"),
        hex_color("#efedf4"),
        hex_color("#e9e7ef"),
        hex_color("#e3e1e9"),
    },
    .on_surface = hex_color("#1b1b21"),
    .on_surface_variant = hex_color("#46464f"),
    .outline = hex_color("#767680"),
    .outline_variant = hex_color("#c6c5d0"),
    .inverse_surface = hex_color("#303036"),
    .inverse_on_surface = hex_color("#f2f0f7"),
    .inverse_primary = hex_color("#b4bcf8"),
    .shadow = hex_color("#000000"),
    .scrim = hex_color("#000000")
};

static theme_colors default_dark_theme = {
    .primary = hex_color("#90c0ff"),
    .on_primary = hex_color("#222c61"),
    .primary_container = hex_color("#394379"),
    .on_primary_container = hex_color("#dee0ff"),
    .secondary = hex_color("#c3c5dd"),
    .on_secondary = hex_color("#2c2f42"),
    .secondary_container = hex_color("#434659"),
    .on_secondary_container = hex_color("#dfe1f9"),
    .tertiary = hex_color("#e5bad8"),
    .on_tertiary = hex_color("#44263e"),
    .tertiary_container = hex_color("#5d3c55"),
    .on_tertiary_container = hex_color("#ffd7f2"),
    .error = hex_color("#ffb4ab"),
    .on_error = hex_color("#690005"),
    .error_container = hex_color("#93000a"),
    .on_error_container = hex_color("#ffdad6"),
    .surface_dim = hex_color("#121318"),
    .surface = hex_color("#121318"),
    .surface_bright = hex_color("#1c1b21"),
    .surface_container_levels = {
        hex_color("#0d0e13"),
        hex_color("#1b1b21"),
        hex_color("#1f1f25"),
        hex_color("#29292f"),
        hex_color("#3c3b42"),
    },
    .on_surface = hex_color("#c6c5d0"),
    .on_surface_variant = hex_color("#c6c5d0"),
    .outline = hex_color("#90909a"),
    .outline_variant = hex_color("#46464f"),
    .inverse_surface = hex_color("#e3e1e9"),
    .inverse_on_surface = hex_color("#303036"),
    .inverse_primary = hex_color("#515b92"),
    .shadow = hex_color("#000000"),
    .scrim = hex_color("#000000")
};

} // namespace alia

#endif
