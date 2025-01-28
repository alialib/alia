#ifndef ALIA_UI_THEME_HPP
#define ALIA_UI_THEME_HPP

#include <array>

#include <alia/ui/color.hpp>

namespace alia {

constexpr static unsigned color_ramp_half_step_count = 4;
constexpr static unsigned color_ramp_step_count
    = 2 * color_ramp_half_step_count + 1;

using color_ramp = std::array<rgb8, color_ramp_step_count>;

struct theme_colors
{
    color_ramp primary;
    color_ramp secondary;
    color_ramp background;
    color_ramp foreground;
    color_ramp warning;
    color_ramp danger;
};

// // prototype based on Material Design 3 color roles
// struct theme_colors
// {
//     // primary
//     rgb8 primary;
//     rgb8 on_primary;
//     rgb8 primary_container;
//     rgb8 on_primary_container;

//     // secondary
//     rgb8 secondary;
//     rgb8 on_secondary;
//     rgb8 secondary_container;
//     rgb8 on_secondary_container;

//     // tertiary
//     rgb8 tertiary;
//     rgb8 on_tertiary;
//     rgb8 tertiary_container;
//     rgb8 on_tertiary_container;

//     // error
//     rgb8 error;
//     rgb8 on_error;
//     rgb8 error_container;
//     rgb8 on_error_container;

//     // surfaces
//     rgb8 surface_dim;
//     rgb8 surface;
//     rgb8 surface_bright;

//     // surface containers, lowest to highest
//     rgb8 surface_container_levels[5];

//     // on surfaces
//     rgb8 on_surface;
//     rgb8 on_surface_variant;

//     // outline
//     rgb8 outline;
//     rgb8 outline_variant;

//     // inverse colors
//     rgb8 inverse_surface;
//     rgb8 inverse_on_surface;
//     rgb8 inverse_primary;

//     // shadow
//     rgb8 shadow;
//     rgb8 scrim;
// };

// static theme_colors violet_light_theme = {
//     .primary = hex_color("#65558f"),
//     .on_primary = hex_color("#ffffff"),
//     .primary_container = hex_color("#e9ddff"),
//     .on_primary_container = hex_color("#201047"),
//     .secondary = hex_color("#625b71"),
//     .on_secondary = hex_color("#ffffff"),
//     .secondary_container = hex_color("#e8def8"),
//     .on_secondary_container = hex_color("#1e192b"),
//     .tertiary = hex_color("#7e5260"),
//     .on_tertiary = hex_color("#ffffff"),
//     .tertiary_container = hex_color("#ffd9e3"),
//     .on_tertiary_container = hex_color("#31101d"),
//     .error = hex_color("#ba1a1a"),
//     .on_error = hex_color("#ffffff"),
//     .error_container = hex_color("#ffdad6"),
//     .on_error_container = hex_color("#410002"),
//     .surface_dim = hex_color("#ded8e0"),
//     .surface = hex_color("#fdf7ff"),
//     .surface_bright = hex_color("#fdf7ff"),
//     .surface_container_levels = {
//         hex_color("#ffffff"),
//         hex_color("#f8f2fa"),
//         hex_color("#f2ecf4"),
//         hex_color("#ece6ee"),
//         hex_color("#e6e1e9"),
//     },
//     .on_surface = hex_color("#1d1b20"),
//     .on_surface_variant = hex_color("#49454e"),
//     .outline = hex_color("#7a757f"),
//     .outline_variant = hex_color("#cac4cf"),
//     .inverse_surface = hex_color("#322f35"),
//     .inverse_on_surface = hex_color("#f5eff7"),
//     .inverse_primary = hex_color("#cfbdfe"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

// static theme_colors violet_dark_theme = {
//     .primary = hex_color("#cfbdfe"),
//     .on_primary = hex_color("#35275d"),
//     .primary_container = hex_color("#4c3d75"),
//     .on_primary_container = hex_color("#e9ddff"),
//     .secondary = hex_color("#cbc2dc"),
//     .on_secondary = hex_color("#332d41"),
//     .secondary_container = hex_color("#4a4458"),
//     .on_secondary_container = hex_color("#e8def8"),
//     .tertiary = hex_color("#efb8c8"),
//     .on_tertiary = hex_color("#4a2532"),
//     .tertiary_container = hex_color("#633b49"),
//     .on_tertiary_container = hex_color("#ffd9e3"),
//     .error = hex_color("#ffb4ab"),
//     .on_error = hex_color("#690005"),
//     .error_container = hex_color("#93000a"),
//     .on_error_container = hex_color("#ffdad6"),
//     .surface_dim = hex_color("#141218"),
//     .surface = hex_color("#141218"),
//     .surface_bright = hex_color("#3b383e"),
//     .surface_container_levels = {
//         hex_color("#0f0d13"),
//         hex_color("#1d1b20"),
//         hex_color("#211f24"),
//         hex_color("#2b292f"),
//         hex_color("#36343a"),
//     },
//     .on_surface = hex_color("#e6e1e9"),
//     .on_surface_variant = hex_color("#cac4cf"),
//     .outline = hex_color("#948f99"),
//     .outline_variant = hex_color("#49454e"),
//     .inverse_surface = hex_color("#e6e1e9"),
//     .inverse_on_surface = hex_color("#322f35"),
//     .inverse_primary = hex_color("#65558f"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

// static theme_colors indigo_light_theme = {
//     .primary = hex_color("#515b92"),
//     .on_primary = hex_color("#ffffff"),
//     .primary_container = hex_color("#dee0ff"),
//     .on_primary_container = hex_color("#0a154b"),
//     .secondary = hex_color("#5b5d72"),
//     .on_secondary = hex_color("#ffffff"),
//     .secondary_container = hex_color("#dfe1f9"),
//     .on_secondary_container = hex_color("#171a2c"),
//     .tertiary = hex_color("#76546d"),
//     .on_tertiary = hex_color("#ffffff"),
//     .tertiary_container = hex_color("#ffd7f2"),
//     .on_tertiary_container = hex_color("#2d1228"),
//     .error = hex_color("#ba1a1a"),
//     .on_error = hex_color("#ffffff"),
//     .error_container = hex_color("#ffdad6"),
//     .on_error_container = hex_color("#410002"),
//     .surface_dim = hex_color("#dbd9e0"),
//     .surface = hex_color("#fbf8ff"),
//     .surface_bright = hex_color("#fbf8ff"),
//     .surface_container_levels = {
//         hex_color("#ffffff"),
//         hex_color("#f5f2fa"),
//         hex_color("#efedf4"),
//         hex_color("#e9e7ef"),
//         hex_color("#e3e1e9"),
//     },
//     .on_surface = hex_color("#1b1b21"),
//     .on_surface_variant = hex_color("#46464f"),
//     .outline = hex_color("#767680"),
//     .outline_variant = hex_color("#c6c5d0"),
//     .inverse_surface = hex_color("#303036"),
//     .inverse_on_surface = hex_color("#f2f0f7"),
//     .inverse_primary = hex_color("#b4bcf8"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

// static theme_colors indigo_dark_theme = {
//     .primary = hex_color("#bac3ff"),
//     .on_primary = hex_color("#222c61"),
//     .primary_container = hex_color("#394379"),
//     .on_primary_container = hex_color("#dee0ff"),
//     .secondary = hex_color("#c3c5dd"),
//     .on_secondary = hex_color("#2c2f42"),
//     .secondary_container = hex_color("#434659"),
//     .on_secondary_container = hex_color("#dfe1f9"),
//     .tertiary = hex_color("#e5bad8"),
//     .on_tertiary = hex_color("#44263e"),
//     .tertiary_container = hex_color("#5d3c55"),
//     .on_tertiary_container = hex_color("#ffd7f2"),
//     .error = hex_color("#ffb4ab"),
//     .on_error = hex_color("#690005"),
//     .error_container = hex_color("#93000a"),
//     .on_error_container = hex_color("#ffdad6"),
//     .surface_dim = hex_color("#121318"),
//     .surface = hex_color("#121318"),
//     .surface_bright = hex_color("#1c1b21"),
//     .surface_container_levels = {
//         hex_color("#0d0e13"),
//         hex_color("#1b1b21"),
//         hex_color("#1f1f25"),
//         hex_color("#29292f"),
//         hex_color("#3c3b42"),
//     },
//     .on_surface = hex_color("#e3e1e9"),
//     .on_surface_variant = hex_color("#c6c5d0"),
//     .outline = hex_color("#90909a"),
//     .outline_variant = hex_color("#46464f"),
//     .inverse_surface = hex_color("#e3e1e9"),
//     .inverse_on_surface = hex_color("#303036"),
//     .inverse_primary = hex_color("#515b92"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

// static theme_colors blue_light_theme = {
//     .primary = hex_color("#35618e"),
//     .on_primary = hex_color("#ffffff"),
//     .primary_container = hex_color("#d1e4ff"),
//     .on_primary_container = hex_color("#001d36"),
//     .secondary = hex_color("#535f70"),
//     .on_secondary = hex_color("#ffffff"),
//     .secondary_container = hex_color("#d6e3f7"),
//     .on_secondary_container = hex_color("#101c2b"),
//     .tertiary = hex_color("#6b5778"),
//     .on_tertiary = hex_color("#ffffff"),
//     .tertiary_container = hex_color("#f2daff"),
//     .on_tertiary_container = hex_color("#251432"),
//     .error = hex_color("#ba1a1a"),
//     .on_error = hex_color("#ffffff"),
//     .error_container = hex_color("#ffdad6"),
//     .on_error_container = hex_color("#410002"),
//     .surface_dim = hex_color("#d8dae0"),
//     .surface = hex_color("#f8f9ff"),
//     .surface_bright = hex_color("#f8f9ff"),
//     .surface_container_levels = {
//         hex_color("#ffffff"),
//         hex_color("#f2f3f9"),
//         hex_color("#eceef4"),
//         hex_color("#e6e8ee"),
//         hex_color("#e1e2e8"),
//     },
//     .on_surface = hex_color("#191c20"),
//     .on_surface_variant = hex_color("#42474e"),
//     .outline = hex_color("#73777f"),
//     .outline_variant = hex_color("#c3c7cf"),
//     .inverse_surface = hex_color("#2e3135"),
//     .inverse_on_surface = hex_color("#eff0f7"),
//     .inverse_primary = hex_color("#a0cafd"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

// static theme_colors blue_dark_theme = {
//     .primary = hex_color("#a0cafd"),
//     .on_primary = hex_color("#003258"),
//     .primary_container = hex_color("#194975"),
//     .on_primary_container = hex_color("#d1e4ff"),
//     .secondary = hex_color("#bac7db"),
//     .on_secondary = hex_color("#253140"),
//     .secondary_container = hex_color("#3b4858"),
//     .on_secondary_container = hex_color("#d6e3f7"),
//     .tertiary = hex_color("#d6bee4"),
//     .on_tertiary = hex_color("#3b2948"),
//     .tertiary_container = hex_color("#523f5f"),
//     .on_tertiary_container = hex_color("#f2daff"),
//     .error = hex_color("#ffb4ab"),
//     .on_error = hex_color("#690005"),
//     .error_container = hex_color("#93000a"),
//     .on_error_container = hex_color("#ffdad6"),
//     .surface_dim = hex_color("#101418"),
//     .surface = hex_color("#101418"),
//     .surface_bright = hex_color("#36393e"),
//     .surface_container_levels = {
//         hex_color("#0b0e13"),
//         hex_color("#191c20"),
//         hex_color("#1d2024"),
//         hex_color("#272a2f"),
//         hex_color("#32353a"),
//     },
//     .on_surface = hex_color("#e1e2e8"),
//     .on_surface_variant = hex_color("#c3c7cf"),
//     .outline = hex_color("#8d9199"),
//     .outline_variant = hex_color("#42474e"),
//     .inverse_surface = hex_color("#e1e2e8"),
//     .inverse_on_surface = hex_color("#2e3135"),
//     .inverse_primary = hex_color("#35618e"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

// static theme_colors pink_light_theme = {
//     .primary = hex_color("#8e4957"),
//     .on_primary = hex_color("#ffffff"),
//     .primary_container = hex_color("#ffd9de"),
//     .on_primary_container = hex_color("#3b0716"),
//     .secondary = hex_color("#75565b"),
//     .on_secondary = hex_color("#ffffff"),
//     .secondary_container = hex_color("#ffd9de"),
//     .on_secondary_container = hex_color("#2c1519"),
//     .tertiary = hex_color("#795831"),
//     .on_tertiary = hex_color("#ffffff"),
//     .tertiary_container = hex_color("#ffddba"),
//     .on_tertiary_container = hex_color("#2b1700"),
//     .error = hex_color("#ba1a1a"),
//     .on_error = hex_color("#ffffff"),
//     .error_container = hex_color("#ffdad6"),
//     .on_error_container = hex_color("#410002"),
//     .surface_dim = hex_color("#e7d6d7"),
//     .surface = hex_color("#fff8f7"),
//     .surface_bright = hex_color("#fff8f7"),
//     .surface_container_levels = {
//         hex_color("#ffffff"),
//         hex_color("#fff0f1"),
//         hex_color("#fbeaeb"),
//         hex_color("#f5e4e5"),
//         hex_color("#f0dee0"),
//     },
//     .on_surface = hex_color("#22191a"),
//     .on_surface_variant = hex_color("#524345"),
//     .outline = hex_color("#847375"),
//     .outline_variant = hex_color("#d6c2c3"),
//     .inverse_surface = hex_color("#382e2f"),
//     .inverse_on_surface = hex_color("#feedee"),
//     .inverse_primary = hex_color("#ffb2be"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

// static theme_colors pink_dark_theme = {
//     .primary = hex_color("#ffb2be"),
//     .on_primary = hex_color("#561d2a"),
//     .primary_container = hex_color("#713340"),
//     .on_primary_container = hex_color("#ffd9de"),
//     .secondary = hex_color("#e5bdc2"),
//     .on_secondary = hex_color("#43292d"),
//     .secondary_container = hex_color("#5c3f43"),
//     .on_secondary_container = hex_color("#ffd9de"),
//     .tertiary = hex_color("#ebbf90"),
//     .on_tertiary = hex_color("#452b08"),
//     .tertiary_container = hex_color("#5f411c"),
//     .on_tertiary_container = hex_color("#ffddba"),
//     .error = hex_color("#ffb4ab"),
//     .on_error = hex_color("#690005"),
//     .error_container = hex_color("#93000a"),
//     .on_error_container = hex_color("#ffdad6"),
//     .surface_dim = hex_color("#191112"),
//     .surface = hex_color("#191112"),
//     .surface_bright = hex_color("#413738"),
//     .surface_container_levels = {
//         hex_color("#140c0d"),
//         hex_color("#22191a"),
//         hex_color("#261d1e"),
//         hex_color("#312829"),
//         hex_color("#3c3233"),
//     },
//     .on_surface = hex_color("#f0dee0"),
//     .on_surface_variant = hex_color("#d6c2c3"),
//     .outline = hex_color("#9f8c8e"),
//     .outline_variant = hex_color("#524345"),
//     .inverse_surface = hex_color("#f0dee0"),
//     .inverse_on_surface = hex_color("#382e2f"),
//     .inverse_primary = hex_color("#8e4957"),
//     .shadow = hex_color("#000000"),
//     .scrim = hex_color("#000000")
// };

} // namespace alia

#endif
