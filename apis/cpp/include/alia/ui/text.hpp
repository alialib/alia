#pragma once

#include <alia/abi/ui/text.h>
#include <alia/impl/context.hpp>
#include <alia/ui/layout/options.hpp>

namespace alia {

// Emit a text leaf.
// A null `style` uses the active font and text color.
// `layout` is applied around the text leaf.
template<layout_like Layout = layout_options>
void
text(
    ui_context ctx,
    alia_text_input_signal content,
    alia_text_style const* style,
    Layout layout = {})
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        alia_text(ctx, raw_code(flags), content, style);
    });
}

// Emit a text leaf with the active font and text color.
template<layout_like Layout = layout_options>
void
text(ui_context ctx, alia_text_input_signal content, Layout layout = {})
{
    text(ctx, content, nullptr, layout);
}

// Emit a text leaf.
// `content` must be a stable null-terminated string whose bytes do not change.
// Pass an `alia_text_input_signal` when the bytes can change.
template<layout_like Layout = layout_options>
void
text(ui_context ctx, char const* content, Layout layout = {})
{
    text(ctx, alia_text_literal(content), layout);
}

// Emit a text leaf with `style`.
// `content` must be a stable null-terminated string whose bytes do not change.
template<layout_like Layout = layout_options>
void
text(
    ui_context ctx,
    char const* content,
    alia_text_style const* style,
    Layout layout = {})
{
    text(ctx, alia_text_literal(content), style, layout);
}

} // namespace alia
