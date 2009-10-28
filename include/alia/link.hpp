#ifndef ALIA_LINK_HPP
#define ALIA_LINK_HPP

#include <alia/layout_interface.hpp>
#include <alia/flags.hpp>
#include <string>

namespace alia {

bool do_link(
    context& ctx,
    char const* text,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS,
    region_id id = auto_id);

bool do_link(
    context& ctx,
    std::string const& text,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS,
    region_id id = auto_id);

}

#endif
