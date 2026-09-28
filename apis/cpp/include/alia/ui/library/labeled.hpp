#pragma once

#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/text.h>
#include <alia/context.h>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/layout/options.hpp>

#include <utility>

namespace alia { namespace detail {

// Emit a single-line labeled bool control.
// `layout` applies to the outer row (the composite).
// The leaf control should use CENTER_Y for vertical alignment.
// Label wrapping is not supported here.
template<layout_like Layout = layout_options, class Control>
alia_element_id
labeled_bool_control(
    context& ctx, char const* label, Layout layout, Control&& control)
{
    alia_box row_box;
    alia_element_id id{};
    row(ctx,
        provide_box(row_box),
        add_default_x_alignment(layout, ALIGN_LEFT),
        [&]() {
            id = std::forward<Control>(control)();
            alia_text(
                &ctx, raw_code(CENTER_Y), alia_text_literal(label), nullptr);
            alia_element_box_region(
                &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
        });
    return id;
}

}} // namespace alia::detail
