#ifndef ALIA_UI_LAYOUT_LIBRARY_HPP
#define ALIA_UI_LAYOUT_LIBRARY_HPP

#include <alia/ui/layout/geometry.hpp>
#include <alia/ui/layout/specification.hpp>
// TODO: This shouldn't be here.
#include <alia/ui/layout/utilities.hpp>

namespace alia {

struct data_traversal;
struct layout_traversal;
struct layout_style_info;

// A spacer simply fills space in a layout.
void
do_spacer(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& layout_spec = default_layout);
// context-based interface
template<class Context>
void
do_spacer(Context& ctx, layout const& layout_spec = default_layout)
{
    do_spacer(get_layout_traversal(ctx), get_data_traversal(ctx), layout_spec);
}

// This version of the spacer records the region that's assigned to it.
void
do_spacer(
    layout_traversal& traversal,
    data_traversal& data,
    layout_box* region,
    layout const& layout_spec = default_layout);
// context-based interface
template<class Context>
void
do_spacer(
    Context& ctx,
    layout_box* region,
    layout const& layout_spec = default_layout)
{
    do_spacer(
        get_layout_traversal(ctx),
        get_data_traversal(ctx),
        region,
        layout_spec);
}

} // namespace alia

#endif
