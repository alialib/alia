#ifndef ALIA_CHECK_BOX_HPP
#define ALIA_CHECK_BOX_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <string>
#include <alia/flags.hpp>

namespace alia {

// accepted flags: all control flags

struct check_box_result : control_result<bool> {};

check_box_result do_check_box(
    context& ctx,
    accessor<bool> const& accessor,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

// check box with text - The text is placed to the right of the check box
// inside a row object, and the check box can be interacted with by clicking
// on either the box itself or the text.
// accepted flags: all control flags, STATIC (for the text), REVERSED

static unsigned const REVERSED = CUSTOM_FLAG_0;

// char const* version
check_box_result do_check_box(
    context& ctx,
    accessor<bool> const& accessor,
    char const* text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

// std::string version
check_box_result do_check_box(
    context& ctx,
    accessor<bool> const& accessor,
    std::string const& text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

}

#endif
