#pragma once

#include <alia/abi/ui/library.h>
#include <alia/context.h>
#include <alia/kernel/macros.hpp>
#include <alia/kernel/signals/core.hpp>
#include <alia/ui/layout/options.hpp>

#include <utility>

namespace alia {

// Emit a collapsible container driven by `expanded`.
// An empty signal is treated as collapsed.
template<view_of<bool> Signal, layout_like Layout = layout_options, class Content>
void
collapsible(
    context& ctx, Signal const& expanded, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        bool const do_content = alia_collapsible_begin(
            &ctx,
            signal_has_value(expanded) && read_signal(expanded),
            raw_code(flags),
            1.f,
            nullptr);
        ALIA_IF_ (&ctx, do_content)
        {
            std::forward<Content>(content)();
        }
        ALIA_END
        alia_collapsible_end(&ctx);
    });
}

template<view_of<bool> Signal, class Content>
void
collapsible(context& ctx, Signal const& expanded, Content&& content)
{
    collapsible(ctx, expanded, default_layout, std::forward<Content>(content));
}

template<view_of<bool> Signal, layout_like Layout = layout_options, class Content>
void
collapsible(
    context& ctx,
    Signal const& expanded,
    Layout layout,
    float offset_factor,
    alia_animated_transition const* transition,
    Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        bool const do_content = alia_collapsible_begin(
            &ctx,
            signal_has_value(expanded) && read_signal(expanded),
            raw_code(flags),
            offset_factor,
            transition);
        ALIA_IF_ (&ctx, do_content)
        {
            std::forward<Content>(content)();
        }
        ALIA_END
        alia_collapsible_end(&ctx);
    });
}

} // namespace alia
