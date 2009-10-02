#ifndef ALIA_COLOR_DISPLAY_HPP
#define ALIA_COLOR_DISPLAY_HPP

#include <alia/layout_interface.hpp>
#include <alia/color.hpp>

namespace alia {

void do_color(context& ctx, rgb8 const& value,
    layout const& layout_spec = default_layout);

}

#endif
