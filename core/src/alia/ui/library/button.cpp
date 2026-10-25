#include <alia/abi/ui/library.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

namespace alia {

struct button_scope
{
    alia_element_id id{};
    alia_button_flags_t flags = 0;
    alia_box box{};
};

static alia_responsive_color
responsive_color_make(
    alia_palette_color base,
    alia_color_op hovered,
    alia_color_op active,
    alia_color_op disabled)
{
    alia_responsive_color color;
    color.base = base;
    color.hovered = hovered;
    color.active = active;
    color.disabled = disabled;
    return color;
}

static void
button_style_apply_filled(
    alia_button_style* style, enum alia_palette_swatch swatch)
{
    alia_palette_color const solid = alia_palette_color_swatch(
        swatch, ALIA_PALETTE_SWATCH_PART_SOLID, 0xff);
    style->fill = responsive_color_make(
        solid,
        alia_color_op_darken(10, 255),
        alia_color_op_darken(20, 255),
        alia_color_op_replace(
            ALIA_PALETTE_COLOR(foundation.background.stronger_2, 0xff)));

    style->label = responsive_color_make(
        alia_palette_color_swatch(
            swatch, ALIA_PALETTE_SWATCH_PART_ON_SOLID, 0xff),
        alia_color_op_none(),
        alia_color_op_none(),
        alia_color_op_replace(
            ALIA_PALETTE_COLOR(foundation.text.weaker_2, 0xff)));

    style->border_width = 0.f;
    style->border = responsive_color_make(
        solid,
        alia_color_op_none(),
        alia_color_op_none(),
        alia_color_op_none());
}

static void
button_style_apply_outline(
    alia_button_style* style, enum alia_palette_swatch swatch)
{
    style->fill = responsive_color_make(
        alia_palette_color_swatch(
            swatch, ALIA_PALETTE_SWATCH_PART_SOLID, 0x00),
        alia_color_op_replace(alia_palette_color_swatch(
            swatch, ALIA_PALETTE_SWATCH_PART_SOLID, 0x20)),
        alia_color_op_replace(alia_palette_color_swatch(
            swatch, ALIA_PALETTE_SWATCH_PART_SOLID, 0x40)),
        alia_color_op_replace(
            ALIA_PALETTE_COLOR(foundation.structural.weaker_3, 0x00)));

    style->label = responsive_color_make(
        alia_palette_color_swatch(swatch, ALIA_PALETTE_SWATCH_PART_TEXT, 0xff),
        alia_color_op_none(),
        alia_color_op_none(),
        alia_color_op_replace(
            ALIA_PALETTE_COLOR(foundation.text.weaker_2, 0xff)));

    if (style->border_width <= 0.f)
        style->border_width = 1.5f;

    // The stroke stays put on hover and press. Disabled swaps to a neutral.
    style->border = responsive_color_make(
        alia_palette_color_swatch(
            swatch, ALIA_PALETTE_SWATCH_PART_OUTLINE, 0xff),
        alia_color_op_none(),
        alia_color_op_none(),
        alia_color_op_replace(
            ALIA_PALETTE_COLOR(foundation.background.stronger_4, 0xff)));
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_button_style_generate(
    alia_button_style* out, alia_style_seeds const* seeds)
{
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    *out = alia_button_style{};
    out->padding_x = s.spacing * 1.4f * s.scale;
    out->padding_y = s.spacing * 0.7f * s.scale;
    out->border_width = 1.5f * s.scale;
    button_style_apply_filled(out, ALIA_PALETTE_SWATCH_PRIMARY);
    out->corner_radius = 6.f * s.scale * s.roundness;
}

void
alia_button_style_apply_swatch(
    alia_button_style* style,
    enum alia_palette_swatch swatch,
    alia_button_chrome_t chrome)
{
    ALIA_ASSERT(style);
    if (chrome == ALIA_BUTTON_CHROME_OUTLINE)
        button_style_apply_outline(style, swatch);
    else
        button_style_apply_filled(style, swatch);
}

alia_button_result_t
alia_button_begin(
    alia_ui_context* ctx,
    alia_button_flags_t flags,
    alia_layout_flags_t layout_flags)
{
    alia_substrate_usage_result const usage
        = alia_substrate_use_memory(alia_ctx_kernel(ctx), 1, 1);
    alia_element_id const id
        = alia_make_element_id(alia_ctx_kernel(ctx), usage);

    alia_button_style const* const style = alia_button_style_active(ctx);
    bool const is_disabled = (flags & ALIA_BUTTON_DISABLED) != 0;
    alia_button_result_t pass_result = ALIA_BUTTON_RESULT_NONE;

    auto& scope = stack_push<button_scope>(alia_ctx_kernel(ctx));
    scope = button_scope{
        .id = id,
        .flags = flags,
        .box = {},
    };

    // Outer margin matches leaf spacing so buttons sit in the same rhythm as
    // text and other controls. FLUSH zeros it. Inner padding is chrome only;
    // PROVIDE_BOX stays on that layer so hit/draw exclude the margin.
    float const layout_spacing
        = (layout_flags & ALIA_FLUSH) != 0 ? 0.f : alia_spacing_active(ctx);
    alia_edge_offsets const margin
        = alia_edge_offsets_make_uniform(layout_spacing);
    alia_edge_offsets const padding
        = alia_edge_offsets_make_xy(style->padding_x, style->padding_y);

    alia_layout_edge_offsets_begin(ctx, margin, layout_flags);
    alia_layout_edge_offsets_begin(ctx, padding, ALIA_PROVIDE_BOX);
    // Row keeps label/content as one flow fragment so PROVIDE_BOX is a single
    // box (edge_offsets alone pass children through to the outer flow).
    alia_layout_row_begin(ctx, 0, 0.f);

    if (!is_refresh_event(*ctx))
    {
        alia_layout_box_array const boxes = alia_layout_consume_box_array(ctx);
        scope.box = boxes.count > 0 ? boxes.boxes[0] : alia_box{};

        alia_interaction_status_t const interaction_status
            = alia_element_get_interaction_status(
                ctx, id, is_disabled ? ALIA_INTERACTION_STATUS_DISABLED : 0);

        alia_text_color_push(
            ctx,
            alia_responsive_color_resolve(
                alia_ctx_palette(ctx), style->label, interaction_status));

        alia_event_category const category = get_event_category(*ctx);
        switch (category)
        {
            case ALIA_CATEGORY_INPUT: {
                if (is_disabled)
                    break;
                // TODO: Wire focus order / keyboard activate once set_focus
                // is implemented. Honor ALIA_BUTTON_SKIP_FOCUS when that
                // lands.
                if (alia_element_detect_click(ctx, id, ALIA_BUTTON_LEFT))
                    pass_result = ALIA_BUTTON_RESULT_ACTIVATED;
                else if (alia_element_detect_click(ctx, id, ALIA_BUTTON_RIGHT))
                    pass_result = ALIA_BUTTON_RESULT_CONTEXT;
                break;
            }
            case ALIA_CATEGORY_DRAWING: {
                alia_palette const* palette = alia_ctx_palette(ctx);
                float const corner = alia_px(ctx, style->corner_radius);
                alia_z_index const z = alia_z(ctx, ALIA_Z_PLANE_BACKGROUND);
                alia_draw_box(
                    ctx,
                    z,
                    scope.box,
                    {.fill_color = alia_responsive_color_resolve(
                         palette, style->fill, interaction_status),
                     .corner_radius = corner,
                     .border_width = alia_px(ctx, style->border_width),
                     .border_color = alia_responsive_color_resolve(
                         palette, style->border, interaction_status)});
                break;
            }
            default:
                break;
        }
    }

    return pass_result;
}

void
alia_button_end(alia_ui_context* ctx)
{
    alia_layout_row_end(ctx);
    alia_layout_edge_offsets_end(ctx);
    alia_layout_edge_offsets_end(ctx);

    if (!is_refresh_event(*ctx))
        alia_text_color_pop(ctx);

    auto scope = stack_pop<button_scope>(alia_ctx_kernel(ctx));

    if (is_refresh_event(*ctx))
        return;

    bool const is_disabled = (scope.flags & ALIA_BUTTON_DISABLED) != 0;

    // Register after children so the button owns the hit (last writer wins).
    alia_element_box_region(
        ctx,
        scope.id,
        &scope.box,
        ALIA_Z_PLANE_CONTENT,
        ALIA_HIT_TEST_MOUSE,
        is_disabled ? ALIA_CURSOR_DEFAULT : ALIA_CURSOR_POINTER);
}

ALIA_EXTERN_C_END
