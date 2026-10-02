#pragma once

#include <alia/abi/ui/library.h>
#include <alia/context.h>
#include <alia/ui/layout/options.hpp>

namespace alia {

// Emit a separator.
template<layout_like Layout = layout_options>
void
separator(context& ctx, Layout layout = {})
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        alia_separator(&ctx, raw_code(flags));
    });
}

} // namespace alia
