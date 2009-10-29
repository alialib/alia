#ifndef ALIA_SEPARATOR_HPP
#define ALIA_SEPARATOR_HPP

#include <alia/forward.hpp>
#include <alia/layout_interface.hpp>
#include <alia/flags.hpp>

namespace alia {

// accepted flags:
// HORIZONTAL, VERTICAL (mutually exclusive, default is HORIZONTAL)
void do_separator(context& ctx, layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS);

}

#endif
