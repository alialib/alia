#pragma once

#include <alia/abi/ui/library.h>
#include <alia/abi/ui/text.h>
#include <alia/context.h>
#include <alia/kernel/signals/c_abi.hpp>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/layout/options.hpp>
#include <alia/ui/library/labeled.hpp>

#include <concepts>

namespace alia {

// Emit a slider.
template<binding_signal Signal, layout_like Layout = layout_options>
    requires std::convertible_to<typename Signal::value_type, double>
          && std::convertible_to<double, typename Signal::value_type>
alia_element_id
slider(
    context& ctx,
    Signal const& value,
    double minimum,
    double maximum,
    double step,
    Layout layout = {},
    bool vertical = false)
{
    alia_element_id id{};
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        auto c = to_inline_c_signal<alia_double_signal>(value);
        id = alia_slider(
            &ctx, &c, minimum, maximum, step, raw_code(flags), vertical);
        write_back_c_signal(&ctx, value, c);
    });
    return id;
}

} // namespace alia
