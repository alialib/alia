#ifndef ALIA_COLOR_CONTROL_HPP
#define ALIA_COLOR_CONTROL_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/color.hpp>

namespace alia {

struct color_control_result
{
    bool changed;
    rgb8 new_value;

    // allows use within if statements without other unintended conversions
    typedef bool color_control_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return changed ? &color_control_result::changed : 0; }
};

color_control_result do_color_control(
    context& ctx,
    accessor<rgb8> const& value,
    unsigned flags = 0,
    layout const& layout_spec = default_layout);

}

#endif
