#include <alia/standard_colors.hpp>
#include <boost/assign/std/vector.hpp>

using namespace boost::assign;

namespace alia {

rgb8 black(0, 0, 0), blue(0, 0, 0xff), cyan(0, 0xff, 0xff),
    dark_blue(0, 0, 0x80), dark_cyan(0, 0x80, 0x80),
    dark_gray(0x40, 0x40, 0x40), dark_green(0, 0x80, 0),
    dark_red(0x80, 0, 0), gray(0x80, 0x80, 0x80), green(0, 0xff, 0),
    light_blue(0x80, 0x80, 0xff), light_gray(0xc0, 0xc0, 0xc0),
    light_green(0x80, 0xff, 0x80), light_red(0xff, 0x80, 0x80),
    magenta(0xff, 0, 0xff), orange(0xff, 0xa5, 0), purple(0x80, 0, 0x80),
    red(0xff, 0, 0), white(0xff, 0xff, 0xff), yellow(0xff, 0xff, 0);

std::vector<named_color> const& get_standard_colors()
{
    static std::vector<named_color> colors;
    if (colors.empty())
    {
        colors.push_back(named_color("magenta", magenta));
        colors.push_back(named_color("purple", purple));
        colors.push_back(named_color("dark blue", dark_blue));
        colors.push_back(named_color("blue", blue));
        colors.push_back(named_color("light blue", light_blue));
        colors.push_back(named_color("cyan", cyan));
        colors.push_back(named_color("dark cyan", dark_cyan));
        colors.push_back(named_color("dark green", dark_green));
        colors.push_back(named_color("green", green));
        colors.push_back(named_color("light green", light_green));
        colors.push_back(named_color("yellow", yellow));
        colors.push_back(named_color("orange", orange));
        colors.push_back(named_color("dark red", dark_red));
        colors.push_back(named_color("red", red));
        colors.push_back(named_color("light red", light_red));
        colors.push_back(named_color("white", white));
        colors.push_back(named_color("light gray", light_gray));
        colors.push_back(named_color("gray", gray));
        colors.push_back(named_color("dark gray", dark_gray));
        colors.push_back(named_color("black", black));
    }
    return colors;
}

int look_up_standard_color(rgb8 const& color)
{
    std::vector<named_color> const& standard_colors = get_standard_colors();
    for (int i = 0; i < int(standard_colors.size()); ++i)
        if (standard_colors[i].color == color)
            return i;
    return -1;
}

}
