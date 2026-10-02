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
    auto opts = as_layout_options(layout);
    alia_vec2f const size = layout_content_size(opts);
    apply_layout(
        ctx, without_size(std::move(opts)), [&](layout_flag_set flags) {
            alia_separator(&ctx, raw_code(flags), size);
        });
}

} // namespace alia
