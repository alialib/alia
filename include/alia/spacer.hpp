#ifndef ALIA_SPACER_HPP
#define ALIA_SPACER_HPP

#include <alia/layout_interface.hpp>

namespace alia {

// A spacer simply creates empty space in the layout. It's useful for filling
// empty grid slots or creating space in rows and columns.

void do_spacer(context& ctx,
    layout const& layout_spec = default_layout);

void do_spacer(context& ctx, box2i* region,
    layout const& layout_spec = default_layout);

}

#endif
