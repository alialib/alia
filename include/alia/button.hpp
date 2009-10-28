#ifndef ALIA_BUTTON_HPP
#define ALIA_BUTTON_HPP

#include <alia/forward.hpp>
#include <alia/layout_interface.hpp>
#include <string>
#include <alia/flags.hpp>

namespace alia {

typedef bool button_result;

// button with text - accepted flags: STATIC (for text), MINIMAL, DISABLED
button_result do_button(
    context& ctx,
    char const* text,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

button_result do_button(
    context& ctx,
    std::string const& text,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

}

#endif
