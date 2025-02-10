#ifndef ALIA_UI_THEME_HPP
#define ALIA_UI_THEME_HPP

#include <array>
#include <optional>

#include <alia/ui/color.hpp>

namespace alia {

struct seed_colors
{
    rgb8 primary;
    rgb8 secondary;
    rgb8 tertiary;
    rgb8 neutral;
    rgb8 warning;
    rgb8 danger;
};

enum class ui_lightness_mode
{
    // dark text on light background
    LIGHT_MODE,
    // light text on dark background
    DARK_MODE
};

constexpr bool
is_light_mode(ui_lightness_mode mode) noexcept
{
    return mode == ui_lightness_mode::LIGHT_MODE;
}

constexpr bool
is_dark_mode(ui_lightness_mode mode) noexcept
{
    return mode == ui_lightness_mode::DARK_MODE;
}

// `augmented_color_info` gives additional (precalculated) info about a color
// that's useful for calculating theme colors and assessing color
// compatibility.
struct augmented_color_info
{
    rgb8 rgb;
    alia::hsl hsl;
    float relative_luminance;
};

augmented_color_info
make_augmented_color_info(rgb8 color);

augmented_color_info
make_augmented_color_info(hsl color);

constexpr int color_ramp_step_count = 11;

using color_ramp = std::array<augmented_color_info, color_ramp_step_count>;

color_ramp
generate_color_ramp(rgb8 color);

struct color_palette
{
    color_ramp primary;
    color_ramp secondary;
    color_ramp tertiary;
    color_ramp neutral;
    color_ramp warning;
    color_ramp danger;
};

color_palette
generate_color_palette(seed_colors const& seeds);

struct contrasting_color_pair
{
    rgb8 main;
    rgb8 contrasting;
};

std::optional<int>
find_darkest_contrasting_index(
    color_ramp const& ramp, rgb8 color, float minimum_contrast_ratio);

std::optional<int>
find_lightest_contrasting_index(
    color_ramp const& ramp, rgb8 color, float minimum_contrast_ratio);

struct color_swatch
{
    static constexpr int stronger_count = 2;
    static constexpr int weaker_count = 1;

    contrasting_color_pair base;
    contrasting_color_pair stronger[stronger_count];
    contrasting_color_pair weaker[weaker_count];
};

struct contrast_parameters
{
    float dark_on_light_ratio;
    float light_on_dark_ratio;
};

color_swatch
generate_color_swatch(
    ui_lightness_mode mode,
    color_ramp const& main_ramp,
    int base_index,
    color_ramp const& contrasting_ramp,
    contrast_parameters const& contrast);

struct theme_colors
{
    color_swatch primary;
    color_swatch secondary;
    color_swatch tertiary;
    color_swatch background;
    color_swatch foreground;
    color_swatch structural;
    color_swatch accent;
    color_swatch warning;
    color_swatch danger;
};

theme_colors
generate_theme_colors(
    ui_lightness_mode mode,
    seed_colors const& seeds,
    contrast_parameters const& contrast);

} // namespace alia

#endif
