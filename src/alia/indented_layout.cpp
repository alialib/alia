#if 0

#include <alia/indented_layout.hpp>

namespace alia {

void indented_layout::begin(context& ctx, unsigned flags,
    layout const& layout_spec)
{
    full_region_.begin(ctx, layout_spec);
    do_spacer(ctx, 
    child_region_.begin(ctx, GROW);
}
void indented_layout::end()
{
    child_region_.end();
    full_region_.end();
}

}

#endif
