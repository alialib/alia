#ifndef DRAGGABLE_SEPARATOR_HPP
#define DRAGGABLE_SEPARATOR_HPP

#include <alia/forward.hpp>
#include <alia/layout_interface.hpp>
#include <alia/flags.hpp>

namespace alia {

bool do_draggable_separator(context& ctx, int* width,
    flag_set flags = NO_FLAGS, layout const& layout_spec = default_layout,
    region_id id = auto_id);

}

#endif
