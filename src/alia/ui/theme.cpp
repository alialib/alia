#include <alia/ui/theme.hpp>

namespace alia {

augmented_color_info
make_augmented_color_info(rgb8 color)
{
    return augmented_color_info{
        color, to_hsl(color), calculate_relative_luminance(color)};
}

augmented_color_info
make_augmented_color_info(hsl color)
{
    auto const rgb = to_rgb8(color);
    return augmented_color_info{rgb, color, calculate_relative_luminance(rgb)};
}

color_ramp
generate_color_ramp(rgb8 color)
{
    auto base = to_hsl(color);

    color_ramp ramp;
    for (int i = 0; i != color_ramp_step_count; ++i)
    {
        float const lightness
            = static_cast<float>(i + 1) / (color_ramp_step_count + 1);
        ramp[i] = make_augmented_color_info(hsl{base.h, base.s, lightness});
    }
    return ramp;
}

color_ramp
generate_neutral_ramp(rgb8 color)
{
    auto base = to_hsl(color);

    float const steps[color_ramp_step_count] = {
        0.04f,
        0.11f,
        0.17f,
        0.27f,
        0.34f,
        0.46f,
        0.64f,
        0.84f,
        0.91f,
        0.96f,
        0.98f,
    };

    color_ramp ramp;
    for (int i = 0; i != color_ramp_step_count; ++i)
    {
        float const lightness = steps[i];
        ramp[i] = make_augmented_color_info(hsl{base.h, base.s, lightness});
    }
    return ramp;
}

color_palette
generate_color_palette(seed_colors const& seeds)
{
    return color_palette{
        generate_color_ramp(seeds.primary),
        generate_color_ramp(seeds.secondary),
        generate_color_ramp(seeds.tertiary),
        generate_neutral_ramp(seeds.neutral),
        generate_color_ramp(seeds.warning),
        generate_color_ramp(seeds.danger),
    };
}

float
relative_luminance_ratio(float luminance_a, float luminance_b)
{
    if (luminance_a < luminance_b)
        return (luminance_b + 0.05f) / (luminance_a + 0.05f);
    else
        return (luminance_a + 0.05f) / (luminance_b + 0.05f);
}

std::optional<int>
find_darkest_contrasting_index(
    color_ramp const& ramp, rgb8 color, float minimum_contrast_ratio)
{
    auto relative_luminance = calculate_relative_luminance(color);
    for (int i = 0; i != color_ramp_step_count; ++i)
    {
        if (relative_luminance_ratio(
                ramp[i].relative_luminance, relative_luminance)
            >= minimum_contrast_ratio)
        {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<int>
find_lightest_contrasting_index(
    color_ramp const& ramp, rgb8 color, float minimum_contrast_ratio)
{
    auto relative_luminance = calculate_relative_luminance(color);
    for (int i = color_ramp_step_count - 1; i >= 0; --i)
    {
        if (relative_luminance_ratio(
                ramp[i].relative_luminance, relative_luminance)
            >= minimum_contrast_ratio)
        {
            return i;
        }
    }
    return std::nullopt;
}

contrasting_color_pair
make_contrasting_color_pair(
    color_ramp const& main_ramp,
    int index,
    color_ramp const& contrasting_ramp,
    ui_lightness_mode mode,
    float minimum_contrast_ratio)
{
    rgb8 main_color;
    if (index < 0)
    {
        main_color = rgb8{0x00, 0x00, 0x00};
    }
    else if (index >= color_ramp_step_count)
    {
        main_color = rgb8{0xff, 0xff, 0xff};
    }
    else
    {
        main_color = main_ramp[index].rgb;
    }
    rgb8 contrasting_color;
    if (mode == ui_lightness_mode::DARK_MODE)
    {
        auto const contrasting_index = find_darkest_contrasting_index(
            contrasting_ramp, main_color, minimum_contrast_ratio);
        if (contrasting_index)
        {
            contrasting_color = contrasting_ramp[*contrasting_index].rgb;
        }
        else
        {
            contrasting_color = rgb8{0xff, 0xff, 0xff};
        }
    }
    else
    {
        auto const contrasting_index = find_lightest_contrasting_index(
            contrasting_ramp, main_color, minimum_contrast_ratio);
        if (contrasting_index)
        {
            contrasting_color = contrasting_ramp[*contrasting_index].rgb;
        }
        else
        {
            contrasting_color = rgb8{0x00, 0x00, 0x00};
        }
    }
    return contrasting_color_pair{main_color, contrasting_color};
}

color_swatch
generate_color_swatch(
    color_ramp const& main_ramp,
    int base_index,
    color_ramp const& contrasting_ramp,
    ui_lightness_mode mode,
    float minimum_contrast_ratio)
{
    color_swatch swatch;
    swatch.base = make_contrasting_color_pair(
        main_ramp, base_index, contrasting_ramp, mode, minimum_contrast_ratio);
    auto const offset = mode == ui_lightness_mode::DARK_MODE ? 1 : -1;
    swatch.stronger[0] = make_contrasting_color_pair(
        main_ramp,
        base_index + offset,
        contrasting_ramp,
        mode,
        minimum_contrast_ratio);
    swatch.stronger[1] = make_contrasting_color_pair(
        main_ramp,
        base_index + 2 * offset,
        contrasting_ramp,
        mode,
        minimum_contrast_ratio);
    swatch.weaker[0] = make_contrasting_color_pair(
        main_ramp,
        base_index - offset,
        contrasting_ramp,
        mode,
        minimum_contrast_ratio);
    return swatch;
}

theme_colors
generate_theme_colors(
    seed_colors const& seeds,
    ui_lightness_mode mode,
    float minimum_contrast_ratio)
{
    theme_colors colors;
    auto const palette = generate_color_palette(seeds);
    colors.primary = generate_color_swatch(
        palette.primary, 5, palette.neutral, mode, minimum_contrast_ratio);
    colors.secondary = generate_color_swatch(
        palette.secondary, 5, palette.neutral, mode, minimum_contrast_ratio);
    colors.tertiary = generate_color_swatch(
        palette.tertiary, 5, palette.neutral, mode, minimum_contrast_ratio);
    colors.background = generate_color_swatch(
        palette.neutral,
        is_dark_mode(mode) ? 2 : 8,
        palette.neutral,
        mode,
        minimum_contrast_ratio);
    colors.structural = generate_color_swatch(
        palette.neutral,
        is_dark_mode(mode) ? 8 : 2,
        palette.neutral,
        mode,
        minimum_contrast_ratio);
    colors.text = generate_color_swatch(
        palette.neutral,
        is_dark_mode(mode) ? 9 : 1,
        palette.neutral,
        mode,
        minimum_contrast_ratio);
    colors.warning = generate_color_swatch(
        palette.warning, 5, palette.neutral, mode, minimum_contrast_ratio);
    colors.danger = generate_color_swatch(
        palette.danger, 5, palette.neutral, mode, minimum_contrast_ratio);
    return colors;
}

} // namespace alia
