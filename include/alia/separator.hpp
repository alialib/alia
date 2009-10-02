#ifndef ALIA_SEPARATOR_HPP
#define ALIA_SEPARATOR_HPP

#include <alia/forward.hpp>
#include <alia/layout_interface.hpp>

namespace alia {

void do_separator(context& ctx, unsigned axis = 0,
    layout const& layout_spec = default_layout);

}

#endif
