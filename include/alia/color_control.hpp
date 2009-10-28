#ifndef ALIA_COLOR_CONTROL_HPP
#define ALIA_COLOR_CONTROL_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/color.hpp>
#include <alia/flags.hpp>

namespace alia {

struct color_control_result : control_result<rgb8> {};

color_control_result do_color_control(
    context& ctx,
    accessor<rgb8> const& value,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS);

}

#endif
