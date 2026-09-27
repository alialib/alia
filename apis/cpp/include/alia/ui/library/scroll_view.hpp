#pragma once

#include <alia/abi/ui/library.h>
#include <alia/base/flags.hpp>
#include <alia/context.h>
#include <alia/ui/layout/options.hpp>

#include <utility>

namespace alia {

ALIA_DEFINE_FLAG_TYPE(alia_scroll_view_flags_t, scroll_view)

#define ALIA_DEFINE_SCROLL_VIEW_FLAG(value, id)                               \
    ALIA_DEFINE_FLAG(scroll_view, value, id)
ALIA_SCROLL_VIEW_FLAGS(ALIA_DEFINE_SCROLL_VIEW_FLAG)
#undef ALIA_DEFINE_SCROLL_VIEW_FLAG

// Emit a scroll view container.
// If neither `SCROLL_VIEW_X` nor `SCROLL_VIEW_Y` is set in `flags`, both axes
// are scrollable.
template<layout_like Layout = layout_options, class Content>
void
scroll_view(
    context& ctx,
    scroll_view_flag_set flags,
    Layout layout,
    Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set layout_flags) {
        alia_scroll_view_begin(&ctx, raw_code(layout_flags), raw_code(flags));
        std::forward<Content>(content)();
        alia_scroll_view_end(&ctx);
    });
}

template<layout_like Layout = layout_options, class Content>
void
scroll_view(context& ctx, Layout layout, Content&& content)
{
    scroll_view(
        ctx,
        scroll_view_flag_set{NO_FLAGS},
        layout,
        std::forward<Content>(content));
}

template<class Content>
void
scroll_view(context& ctx, scroll_view_flag_set flags, Content&& content)
{
    scroll_view(ctx, flags, default_layout, std::forward<Content>(content));
}

template<class Content>
void
scroll_view(context& ctx, Content&& content)
{
    scroll_view(
        ctx,
        scroll_view_flag_set{NO_FLAGS},
        default_layout,
        std::forward<Content>(content));
}

} // namespace alia
