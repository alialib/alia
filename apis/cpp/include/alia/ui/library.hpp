#pragma once

#include <alia/abi/ui/library.h>
#include <alia/context.h>
#include <alia/ui/layout/flags.hpp>

#include <utility>

namespace alia {

// COLLAPSIBLE

// TODO: Use alia::if_ to omit child UI when fully collapsed.
// TODO: Use C++ signals.

template<class Content>
void
collapsible(context& ctx, alia_bool_signal* expanded, Content&& content)
{
    alia_ui_collapsible_begin(&ctx, expanded, 0, 1.f, nullptr);
    std::forward<Content>(content)();
    alia_ui_collapsible_end(&ctx);
}

template<class Content>
void
collapsible(
    context& ctx,
    alia_bool_signal* expanded,
    layout_flag_set column_flags,
    Content&& content)
{
    alia_ui_collapsible_begin(
        &ctx, expanded, raw_code(column_flags), 1.f, nullptr);
    std::forward<Content>(content)();
    alia_ui_collapsible_end(&ctx);
}

template<class Content>
void
collapsible(
    context& ctx,
    alia_bool_signal* expanded,
    layout_flag_set column_flags,
    float offset_factor,
    alia_animated_transition const* transition,
    Content&& content)
{
    alia_ui_collapsible_begin(
        &ctx, expanded, raw_code(column_flags), offset_factor, transition);
    std::forward<Content>(content)();
    alia_ui_collapsible_end(&ctx);
}

} // namespace alia
