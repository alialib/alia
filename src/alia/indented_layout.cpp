#include <alia/indented_layout.hpp>
#include <alia/spacer.hpp>

namespace alia {

void do_indent(context& ctx)
{
    do_spacer(ctx, width(4, CHARS));
}

void indented_layout::begin(context& ctx, layout const& layout_spec,
    flag_set flags)
{
    full_region_.begin(ctx, layout_spec);
    do_spacer(ctx, width(4, CHARS));
    child_region_.begin(ctx, GROW);
}
void indented_layout::end()
{
    child_region_.end();
    full_region_.end();
}

}
