#ifndef ALIA_UI_STYLING_HPP
#define ALIA_UI_STYLING_HPP

#include <alia/ui/color.hpp>

class SkFont;

namespace alia {

struct font_info
{
    // TODO: Use a wrapper around this to allow for more generic font handling
    // in the future.
    SkFont* font = nullptr;

    auto
    operator<=>(font_info const&) const
        = default;
};

struct style_info
{
    font_info font;
    rgba8 color;

    auto
    operator<=>(style_info const&) const
        = default;
};

} // namespace alia

#endif
