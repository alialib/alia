#pragma once

#include <alia/abi/kernel/signal.h>
#include <alia/abi/ui/library.h>
#include <alia/abi/ui/text.h>
#include <alia/context.h>
#include <alia/kernel/actions/core.hpp>
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
    bool const do_content
        = alia_ui_collapsible_begin(&ctx, expanded, 0, 1.f, nullptr);
    ALIA_IF_ (&ctx, do_content)
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
    bool const do_content = alia_ui_collapsible_begin(
        &ctx, expanded, raw_code(column_flags), 1.f, nullptr);
    ALIA_IF_ (&ctx, do_content)
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
    bool const do_content = alia_ui_collapsible_begin(
        &ctx, expanded, raw_code(column_flags), offset_factor, transition);
    ALIA_IF_ (&ctx, do_content)
        std::forward<Content>(content)();
    alia_ui_collapsible_end(&ctx);
}

// BUTTON

// Emit a labeled button that performs `on_click` when activated. The button
// is disabled while the action is not ready. Label color follows the button
// style via `alia_ui_button_begin`'s active text-style override.
inline void
button(
    context& ctx,
    char const* label,
    action<> const& on_click,
    layout_flag_set layout_flags = NO_FLAGS)
{
    alia_button_flags_t flags = 0;
    if (!action_is_ready(on_click))
        flags |= ALIA_BUTTON_DISABLED;
    alia_button_result_t const result
        = alia_ui_button_begin(&ctx, flags, raw_code(layout_flags));
    alia_text(&ctx, 0, alia_text_literal(label), nullptr);
    alia_ui_button_end(&ctx);
    if (result == ALIA_BUTTON_RESULT_ACTIVATED)
        perform_action(on_click);
}

} // namespace alia
