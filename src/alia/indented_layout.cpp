#include <alia/indented_layout.hpp>
#include <alia/spacer.hpp>

namespace alia {

void do_indent(context& ctx)
{
    // temporarily hard-coded to be the width of a node expander
    do_spacer(ctx, width(15, PIXELS));
}

void indented_layout::begin(context& ctx, layout const& layout_spec,
    flag_set flags)
{
    full_region_.begin(ctx, layout_spec);
    do_indent(ctx);
    child_region_.begin(ctx, GROW);
}
void indented_layout::end()
{
    child_region_.end();
    full_region_.end();
}

}
