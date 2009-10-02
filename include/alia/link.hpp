#ifndef ALIA_LINK_HPP
#define ALIA_LINK_HPP

#include <alia/layout_interface.hpp>
#include <string>

namespace alia {

bool do_link(
    context& ctx,
    char const* text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

bool do_link(
    context& ctx,
    std::string const& text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

}

#endif
