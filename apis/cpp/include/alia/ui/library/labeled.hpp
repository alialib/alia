#pragma once

#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/styling/api.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/context.hpp>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/layout/options.hpp>

#include <utility>

namespace alia { namespace detail {

// Emit a single-line labeled bool control.
// `layout` applies to the outer row (the composite).
// The leaf control should use `CENTER_Y` for vertical alignment.
// Label wrapping is not supported here.
template<layout_like Layout = layout_options, class Control>
alia_element_id
labeled_bool_control(
    ui_context ctx, char const* label, Layout layout, Control&& control)
{
    alia_box row_box;
    alia_element_id id{};
    float const spacing = alia_spacing_active(ctx);
    row(ctx,
        provide_box(row_box),
        add_default_x_alignment(layout, ALIGN_LEFT),
        [&]() {
            id = std::forward<Control>(control)();

            alia_layout_edge_offsets_begin(
                ctx, alia_edge_offsets{0, spacing, spacing, spacing}, 0);
            alia_text(
                ctx,
                raw_code(FLUSH | CENTER_Y),
                alia_text_literal(label),
                nullptr);
            alia_layout_edge_offsets_end(ctx);

            alia_element_box_region(
                ctx,
                id,
                &row_box,
                ALIA_Z_PLANE_CONTENT,
                ALIA_HIT_TEST_MOUSE,
                ALIA_CURSOR_DEFAULT);
        });
    return id;
}

}} // namespace alia::detail
