#ifndef ALIA_ICON_BUTTON_HPP
#define ALIA_ICON_BUTTON_HPP

#include <alia/layout_interface.hpp>
#include <alia/artist.hpp>

namespace alia {

typedef bool icon_button_result;

icon_button_result
do_icon_button(
    context& ctx,
    standard_icon icon,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

}

#endif
