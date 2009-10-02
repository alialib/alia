#ifndef ALIA_BUTTON_HPP
#define ALIA_BUTTON_HPP

#include <alia/forward.hpp>
#include <alia/layout_interface.hpp>
#include <string>

namespace alia {

typedef bool button_result;

// button with text - accepted flags: STATIC (for text), MINIMAL, DISABLED
button_result do_button(
    context& ctx,
    char const* text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

button_result do_button(
    context& ctx,
    std::string const& text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

}

#endif
