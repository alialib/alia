#pragma once

#include <alia/abi/ui/library.h>
#include <alia/abi/ui/styling/color_program.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/context.hpp>
#include <alia/impl/ui/styling/color_program.hpp>
#include <alia/kernel/actions/core.hpp>
#include <alia/ui/layout/options.hpp>

#include <utility>

namespace alia {

// button chrome style
enum class button_chrome
{
    filled,
    outline,
    gradient,
};

inline alia_style_channel_ref
channel_from_slot(alia_style_color_slot slot)
{
    return alia_style_channel_from_slot(slot);
}

inline alia_style_channel_ref
channel_value(float value)
{
    return alia_style_channel_from_value(value);
}

// factory recipe describing a standard button chrome variant
struct button_variant
{
    button_chrome chrome = button_chrome::filled;
    color::tone tone = color::tone::match;
    alia_style_channel_ref hue = channel_from_slot(ALIA_STYLE_COLOR_PRIMARY);
    alia_style_channel_ref chroma = channel_from_slot(ALIA_STYLE_COLOR_PRIMARY);
};

namespace detail {

inline alia_button_chrome_t
to_c_chrome(button_chrome chrome)
{
    switch (chrome)
    {
        case button_chrome::filled:
            return ALIA_BUTTON_CHROME_FILLED;
        case button_chrome::outline:
            return ALIA_BUTTON_CHROME_OUTLINE;
        case button_chrome::gradient:
            return ALIA_BUTTON_CHROME_GRADIENT;
    }
    return ALIA_BUTTON_CHROME_FILLED;
}

inline alia_button_recipe
to_c_recipe(button_variant const& variant)
{
    alia_button_recipe recipe{};
    recipe.chrome = to_c_chrome(variant.chrome);
    recipe.tone = (alia_color_tone_t) variant.tone;
    recipe.hue = variant.hue;
    recipe.chroma = variant.chroma;
    return recipe;
}

} // namespace detail

// Run `content` while the active button style matches `variant`.
template<class Content>
void
with_button_variant(ui_context ctx, button_variant variant, Content&& content)
{
    alia_button_style* style = alia_button_style_active(ctx);
    alia_button_style const saved = *style;
    // Emit a pass-local override into the open scratch bump.
    // (The program steps are allocated in the pass's scratch space.)
    alia_color_program_emitter emitter = alia_color_program_emitter_begin(
        alia_ctx_scratch(alia_ctx_kernel(ctx)));
    alia_button_recipe const recipe = detail::to_c_recipe(variant);
    alia_button_style_apply_recipe(
        style, &recipe, &alia_ctx_style(ctx)->environment->seeds, &emitter);
    std::forward<Content>(content)();
    *alia_button_style_active(ctx) = saved;
}

// Emit a button container.
// Return what happened on this pass.
template<layout_like Layout = layout_options, class Content>
alia_button_result_t
button(
    ui_context ctx,
    alia_button_flags_t flags,
    Layout layout,
    Content&& content)
{
    alia_button_result_t result = ALIA_BUTTON_RESULT_NONE;
    apply_layout(ctx, layout, [&](layout_flag_set layout_flags) {
        result = alia_button_begin(ctx, flags, raw_code(layout_flags));
        std::forward<Content>(content)();
        alia_button_end(ctx);
    });
    return result;
}

template<layout_like Layout = layout_options, class Content>
alia_button_result_t
button(
    ui_context ctx,
    alia_button_flags_t flags,
    Layout layout,
    button_variant variant,
    Content&& content)
{
    alia_button_result_t result = ALIA_BUTTON_RESULT_NONE;
    with_button_variant(ctx, variant, [&] {
        result = button(ctx, flags, layout, std::forward<Content>(content));
    });
    return result;
}

// Emit a button container that posts `on_click` when activated.
// Disable the button while the action is not ready.
template<layout_like Layout = layout_options, class Content>
void
button(
    ui_context ctx, action<> const& on_click, Layout layout, Content&& content)
{
    alia_button_flags_t flags = 0;
    if (!action_is_ready(on_click))
        flags |= ALIA_BUTTON_DISABLED;
    if (button(ctx, flags, layout, std::forward<Content>(content))
        == ALIA_BUTTON_RESULT_ACTIVATED)
    {
        post_action(ctx, on_click);
    }
}

template<layout_like Layout = layout_options, class Content>
void
button(
    ui_context ctx,
    action<> const& on_click,
    Layout layout,
    button_variant variant,
    Content&& content)
{
    with_button_variant(ctx, variant, [&] {
        button(ctx, on_click, layout, std::forward<Content>(content));
    });
}

// Emit a labeled button that posts `on_click` when activated.
template<layout_like Layout = layout_options>
void
button(
    ui_context ctx,
    char const* label,
    action<> const& on_click,
    Layout layout = {})
{
    button(ctx, on_click, layout, [&] {
        // Use `FLUSH` here to reduce the padding between the button and the
        // label.
        alia_text(ctx, raw_code(FLUSH), alia_text_literal(label), nullptr);
    });
}

template<layout_like Layout = layout_options>
void
button(
    ui_context ctx,
    char const* label,
    action<> const& on_click,
    Layout layout,
    button_variant variant)
{
    button(ctx, on_click, layout, variant, [&] {
        alia_text(ctx, raw_code(FLUSH), alia_text_literal(label), nullptr);
    });
}

inline void
button(
    ui_context ctx,
    char const* label,
    action<> const& on_click,
    button_variant variant)
{
    button(ctx, label, on_click, default_layout, variant);
}

} // namespace alia
