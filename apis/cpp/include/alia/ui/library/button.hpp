#pragma once

#include <alia/abi/ui/library.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/text.h>
#include <alia/context.h>
#include <alia/kernel/actions/core.hpp>
#include <alia/ui/layout/flags.hpp>

#include <utility>

namespace alia {

// Semantic palette swatch for button chrome variants.
enum class swatch
{
    focus,
    selection,
    primary,
    secondary,
    success,
    warning,
    danger,
    info,
};

// Button chrome style.
enum class button_chrome
{
    filled,
    outline,
};

// swatch and chrome pair describing a button variant.
struct button_variant
{
    swatch swatch{};
    button_chrome chrome = button_chrome::filled;
};

namespace detail {

inline alia_palette_swatch
to_c_swatch(swatch s)
{
    switch (s)
    {
        case swatch::focus:
            return ALIA_PALETTE_SWATCH_FOCUS;
        case swatch::selection:
            return ALIA_PALETTE_SWATCH_SELECTION;
        case swatch::primary:
            return ALIA_PALETTE_SWATCH_PRIMARY;
        case swatch::secondary:
            return ALIA_PALETTE_SWATCH_SECONDARY;
        case swatch::success:
            return ALIA_PALETTE_SWATCH_SUCCESS;
        case swatch::warning:
            return ALIA_PALETTE_SWATCH_WARNING;
        case swatch::danger:
            return ALIA_PALETTE_SWATCH_DANGER;
        case swatch::info:
            return ALIA_PALETTE_SWATCH_INFO;
    }
    return ALIA_PALETTE_SWATCH_PRIMARY;
}

inline alia_button_chrome_t
to_c_chrome(button_chrome chrome)
{
    switch (chrome)
    {
        case button_chrome::filled:
            return ALIA_BUTTON_CHROME_FILLED;
        case button_chrome::outline:
            return ALIA_BUTTON_CHROME_OUTLINE;
    }
    return ALIA_BUTTON_CHROME_FILLED;
}

struct button_variant_scope
{
    context& ctx;
    alia_button_style saved{};

    button_variant_scope(context& ctx_, button_variant variant)
        : ctx(ctx_)
    {
        alia_button_style* style = alia_button_style_active(&ctx);
        saved = *style;
        alia_button_style_apply_swatch(
            style,
            to_c_swatch(variant.swatch),
            to_c_chrome(variant.chrome));
    }

    ~button_variant_scope()
    {
        *alia_button_style_active(&ctx) = saved;
    }

    button_variant_scope(button_variant_scope const&) = delete;
    button_variant_scope&
    operator=(button_variant_scope const&)
        = delete;
};

} // namespace detail

// Run `content` while the active button style matches `variant`.
template<class Content>
void
with_button_variant(context& ctx, button_variant variant, Content&& content)
{
    detail::button_variant_scope const scope(ctx, variant);
    std::forward<Content>(content)();
}

// Emit a button container. Returns what happened on this pass.
template<class Content>
alia_button_result_t
button(
    context& ctx,
    alia_button_flags_t flags,
    layout_flag_set layout_flags,
    Content&& content)
{
    alia_button_result_t const result
        = alia_ui_button_begin(&ctx, flags, raw_code(layout_flags));
    std::forward<Content>(content)();
    alia_ui_button_end(&ctx);
    return result;
}

template<class Content>
alia_button_result_t
button(
    context& ctx,
    alia_button_flags_t flags,
    layout_flag_set layout_flags,
    button_variant variant,
    Content&& content)
{
    alia_button_result_t result = ALIA_BUTTON_RESULT_NONE;
    with_button_variant(ctx, variant, [&] {
        result = button(
            ctx, flags, layout_flags, std::forward<Content>(content));
    });
    return result;
}

// Emit a button container that performs `on_click` when activated. The button
// is disabled while the action is not ready.
template<class Content>
void
button(
    context& ctx,
    action<> const& on_click,
    layout_flag_set layout_flags,
    Content&& content)
{
    alia_button_flags_t flags = 0;
    if (!action_is_ready(on_click))
        flags |= ALIA_BUTTON_DISABLED;
    if (button(ctx, flags, layout_flags, std::forward<Content>(content))
        == ALIA_BUTTON_RESULT_ACTIVATED)
        perform_action(on_click);
}

template<class Content>
void
button(
    context& ctx,
    action<> const& on_click,
    layout_flag_set layout_flags,
    button_variant variant,
    Content&& content)
{
    with_button_variant(ctx, variant, [&] {
        button(ctx, on_click, layout_flags, std::forward<Content>(content));
    });
}

// Emit a labeled button that performs `on_click` when activated.
inline void
button(
    context& ctx,
    char const* label,
    action<> const& on_click,
    layout_flag_set layout_flags = NO_FLAGS)
{
    button(ctx, on_click, layout_flags, [&] {
        alia_text(&ctx, 0, alia_text_literal(label), nullptr);
    });
}

inline void
button(
    context& ctx,
    char const* label,
    action<> const& on_click,
    layout_flag_set layout_flags,
    button_variant variant)
{
    button(ctx, on_click, layout_flags, variant, [&] {
        alia_text(&ctx, 0, alia_text_literal(label), nullptr);
    });
}

inline void
button(
    context& ctx,
    char const* label,
    action<> const& on_click,
    button_variant variant)
{
    button(ctx, label, on_click, NO_FLAGS, variant);
}

} // namespace alia
