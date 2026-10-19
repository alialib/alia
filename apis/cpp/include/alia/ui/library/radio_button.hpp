#pragma once

#include <alia/abi/ui/library.h>
#include <alia/context.h>
#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/c_abi.hpp>
#include <alia/ui/layout/options.hpp>
#include <alia/ui/library/labeled.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace alia {

// Emit a radio button bound to `value`.
// The button is selected when `value` is true.
// Clicking it sets `value` to true.
// (There is no way to set `value` to false via the UI.)
template<binding_of<bool> Signal, layout_like Layout = layout_options>
alia_element_id
radio_button(ui_context& ctx, Signal const& value, Layout layout = {})
{
    alia_element_id id{};
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        auto c = to_inline_c_signal<alia_bool_signal>(value);
        id = alia_radio_button(&ctx, &c, raw_code(flags));
        write_back_c_signal(&ctx.kernel, value, c);
    });
    return id;
}

// Emit a radio button with an associated label. The two share a hit region.
template<binding_of<bool> Signal, layout_like Layout = layout_options>
alia_element_id
radio_button(
    ui_context& ctx,
    Signal const& value,
    char const* label,
    Layout layout = {})
{
    return detail::labeled_bool_control(ctx, label, layout, [&] {
        return radio_button(ctx, value, CENTER_Y);
    });
}

// Emit a radio button for `index` within the `selected` group.
// The button is selected when `selected` equals `index`. Activating it writes
// `index` into `selected`.
template<class Selected, class Index, layout_like Layout = layout_options>
    requires view_signal<Selected> && sink_signal<Selected>
          && (!layout_like<std::remove_cvref_t<Index>>)
          && (!std::convertible_to<std::remove_cvref_t<Index>, char const*>)
alia_element_id
radio_button(
    ui_context& ctx, Selected const& selected, Index index, Layout layout = {})
{
    return radio_button(
        ctx, make_radio_signal(selected, std::move(index)), layout);
}

// Emit a labeled radio button for `index` within the `selected` group.
// The button and its label share a hit region.
template<class Selected, class Index, layout_like Layout = layout_options>
    requires view_signal<Selected> && sink_signal<Selected>
          && (!layout_like<std::remove_cvref_t<Index>>)
alia_element_id
radio_button(
    ui_context& ctx,
    Selected const& selected,
    Index index,
    char const* label,
    Layout layout = {})
{
    return detail::labeled_bool_control(ctx, label, layout, [&] {
        return radio_button(ctx, selected, std::move(index), CENTER_Y);
    });
}

} // namespace alia
