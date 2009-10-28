#ifndef ALIA_SEPARATOR_HPP
#define ALIA_SEPARATOR_HPP

#include <alia/forward.hpp>
#include <alia/layout_interface.hpp>
#include <alia/flags.hpp>

namespace alia {

void do_separator(context& ctx, flag_set axis = NO_FLAGS,
    layout const& layout_spec = default_layout);

}

#endif
