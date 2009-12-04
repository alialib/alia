#ifndef ALIA_STANDARD_COLORS_HPP
#define ALIA_STANDARD_COLORS_HPP

#include <alia/color.hpp>
#include <vector>
#include <string>

namespace alia {

// A list of standard colors for easier use in code.
extern rgb8 black, blue, cyan, dark_blue, dark_cyan, dark_gray, dark_green,
    dark_red, gray, green, light_blue, light_gray, light_green, light_red,
    magenta, orange, purple, red, white, yellow;

// The list of standard colors is also available in a form that makes the color
// name available at run-time...

struct named_color
{
    named_color(std::string n, rgb8 const& c)
      : name(n), color(c) {}

    std::string name;
    rgb8 color;
};

// Get the list of standard colors.
std::vector<named_color> const& get_standard_colors();

// Looks up the given color in the standard color list.  If found, it returns
// an index to the color.  Otherwise, returns -1.
int look_up_standard_color(rgb8 const& color);

// Looks up the given name in the standard color list.  If found, it returns
// an index to the color.  Otherwise, returns -1.
int look_up_standard_color(std::string const& name);

}

#endif
