#pragma once

#include <alia/abi/ui/library.h>
#include <alia/context.h>
#include <alia/kernel/signals/c_abi.hpp>
#include <alia/ui/layout/options.hpp>
#include <alia/ui/library/labeled.hpp>

namespace alia {

// Emit a checkbox.
template<binding_of<bool> Signal, layout_like Layout = layout_options>
alia_element_id
checkbox(context& ctx, Signal const& value, Layout layout = {})
{
    alia_element_id id{};
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        auto c = to_inline_c_signal<alia_bool_signal>(value);
        id = alia_checkbox(&ctx, &c, raw_code(flags));
        write_back_c_signal(&ctx, value, c);
    });
    return id;
}

// Emit a checkbox with an associated label. The two share a hit region.
template<binding_of<bool> Signal, layout_like Layout = layout_options>
alia_element_id
checkbox(
    context& ctx, Signal const& value, char const* label, Layout layout = {})
{
    return detail::labeled_bool_control(
        ctx, label, layout, [&] { return checkbox(ctx, value); });
}

} // namespace alia
