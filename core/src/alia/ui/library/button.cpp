#include <alia/abi/ui/library.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

namespace alia {

struct button_scope
{
    alia_element_id id{};
    alia_button_flags_t flags = 0;
    alia_box box{};
    alia_palette_color saved_text_color{};
    alia_z_index saved_z_base = 0;
};

static void
button_pick_base(
    alia_button_style const* style,
    bool disabled,
    alia_interaction_status_t status,
    alia_palette_color* fill,
    alia_palette_color* label,
    alia_palette_color* border)
{
    if (disabled)
    {
        *fill = style->fill_disabled;
        *label = style->label_disabled;
        *border = style->border_disabled;
        return;
    }

    *fill = style->fill;
    *label = style->label;
    *border = style->border;

    // Outline rest uses transparent fill + text-colored label. Hover/active
    // paint the swatch solid via highlight, so flip the label to on-solid.
    // `highlight_hovered` is the SOLID part; ON_SOLID is the next part.
    if (style->fill.alpha == 0
        && (status
            & (ALIA_INTERACTION_STATUS_HOVERED
               | ALIA_INTERACTION_STATUS_ACTIVE)))
    {
        *label = alia_palette_color_make(
            (uint8_t) (style->highlight_hovered.index
                       + ALIA_PALETTE_SWATCH_PART_ON_SOLID
                       - ALIA_PALETTE_SWATCH_PART_SOLID),
            0xff);
    }
}

static alia_palette_color
button_pick_highlight(
    alia_button_style const* style, alia_interaction_status_t status)
{
    if (status & ALIA_INTERACTION_STATUS_DISABLED)
        return alia_palette_color_make(0, 0);
    if (status & ALIA_INTERACTION_STATUS_ACTIVE)
        return style->highlight_active;
    if (status & ALIA_INTERACTION_STATUS_HOVERED)
        return style->highlight_hovered;
    return alia_palette_color_make(0, 0);
}

static alia_box
button_union_boxes(alia_layout_box_array const& boxes)
{
    if (boxes.count == 0 || boxes.boxes == nullptr)
        return alia_box{};
    alia_box out = boxes.boxes[0];
    for (uint32_t i = 1; i < boxes.count; ++i)
        out = alia_box_union(out, boxes.boxes[i]);
    return out;
}

static void
button_style_apply_filled(
    alia_button_style* style, enum alia_palette_swatch swatch)
{
    style->fill = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_SOLID),
        0xff);
    style->fill_disabled = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
            ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
        0xff);

    style->label = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_ON_SOLID),
        0xff);
    style->label_disabled = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_TEXT,
            ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
        0xff);

    style->border_width = 0.f;
    style->border = style->fill;
    style->border_disabled = style->fill_disabled;
    style->corner_radius = 0.f;

    // Soft ink wash over the solid fill.
    style->highlight_hovered = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_ON_SOLID),
        0x28);
    style->highlight_active = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_ON_SOLID),
        0x45);
}

static void
button_style_apply_outline(
    alia_button_style* style, enum alia_palette_swatch swatch)
{
    // Transparent base; hover fills with the swatch solid.
    style->fill = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_SUBTLE),
        0x00);
    style->fill_disabled = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
            ALIA_PALETTE_RAMP_LEVEL_WEAKER_3),
        0x00);

    style->label = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_TEXT),
        0xff);
    style->label_disabled = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_TEXT,
            ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
        0xff);

    if (style->border_width <= 0.f)
        style->border_width = 1.5f;

    style->border = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_OUTLINE),
        0xff);
    style->border_disabled = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
            ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
        0xff);

    // Keep a slight round on outline chrome (filled uses square corners).
    if (style->corner_radius <= 0.f)
        style->corner_radius = 6.f;

    style->highlight_hovered = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_SOLID),
        0xff);
    style->highlight_active = alia_palette_color_make(
        alia_palette_index_swatch(swatch, ALIA_PALETTE_SWATCH_PART_SOLID),
        0xff);
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
    out->corner_radius = 0.f;
    out->border_width = 1.5f * s.scale;
    button_style_apply_filled(out, ALIA_PALETTE_SWATCH_PRIMARY);
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
alia_ui_button_begin(
    alia_context* ctx,
    alia_button_flags_t flags,
    alia_layout_flags_t layout_flags)
{
    alia_substrate_usage_result const usage
        = alia_substrate_use_memory(ctx, 1, 1);
    alia_element_id const id = alia_make_element_id(ctx, usage);

    alia_button_style const* const style = alia_button_style_active(ctx);
    bool const is_disabled = (flags & ALIA_BUTTON_DISABLED) != 0;
    alia_button_result_t pass_result = ALIA_BUTTON_RESULT_NONE;

    auto& scope = stack_push<button_scope>(ctx);
    scope = button_scope{
        .id = id,
        .flags = flags,
        .box = {},
        .saved_text_color = {},
        .saved_z_base = ctx->geometry->z_base,
    };

    // Outer margin matches leaf spacing so buttons sit in the same rhythm as
    // text and other controls. FLUSH zeros it. Inner padding is chrome only;
    // PROVIDE_BOX stays on that layer so hit/draw exclude the margin.
    float const layout_spacing = (layout_flags & ALIA_FLUSH) != 0
                                   ? 0.f
                                   : alia_layout_style_active(ctx)->spacing;
    alia_edge_offsets const margin
        = alia_edge_offsets_make_uniform(layout_spacing);
    alia_edge_offsets const padding = alia_edge_offsets_make_xy(
        alia_px(ctx, style->padding_x), alia_px(ctx, style->padding_y));

    if (is_refresh_event(*ctx))
    {
        alia_layout_edge_offsets_begin(ctx, margin, layout_flags);
        alia_layout_edge_offsets_begin(ctx, padding, ALIA_PROVIDE_BOX);
    }
    else
    {
        alia_layout_box_array const boxes
            = alia_layout_consume_box_array(ctx);
        scope.box = button_union_boxes(boxes);

        alia_interaction_status_t const interaction_status
            = alia_element_get_interaction_status(
                ctx,
                id,
                is_disabled ? ALIA_INTERACTION_STATUS_DISABLED : 0);

        alia_palette_color fill_c, label_c, border_c;
        button_pick_base(
            style,
            is_disabled,
            interaction_status,
            &fill_c,
            &label_c,
            &border_c);
        alia_palette_color const highlight_c
            = button_pick_highlight(style, interaction_status);

        alia_text_style* text_style = alia_text_style_active(ctx);
        scope.saved_text_color = text_style->color;
        text_style->color = label_c;

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
                alia_draw_box(
                    ctx,
                    ctx->geometry->z_base,
                    scope.box,
                    {.fill_color
                     = alia_palette_color_resolve(palette, fill_c),
                     .corner_radius = corner,
                     .border_width = alia_px(ctx, style->border_width),
                     .border_color
                     = alia_palette_color_resolve(palette, border_c)});
                if (highlight_c.alpha != 0)
                {
                    alia_draw_box(
                        ctx,
                        ctx->geometry->z_base,
                        scope.box,
                        {.fill_color = alia_palette_color_resolve(
                             palette, highlight_c),
                         .corner_radius = corner,
                         .border_width = 0.f,
                         .border_color = alia_srgba8_make(0, 0, 0, 0)});
                }
                // Content draws above the chrome.
                ++ctx->geometry->z_base;
                break;
            }
            default:
                break;
        }

        // Non-refresh edge_offsets begin is a no-op; keep the walk aligned
        // by still entering the same begin/end pairing.
        alia_layout_edge_offsets_begin(ctx, margin, 0);
        alia_layout_edge_offsets_begin(ctx, padding, 0);
    }

    return pass_result;
}

void
alia_ui_button_end(alia_context* ctx)
{
    alia_layout_edge_offsets_end(ctx);
    alia_layout_edge_offsets_end(ctx);

    auto scope = stack_pop<button_scope>(ctx);

    if (is_refresh_event(*ctx))
        return;

    bool const is_disabled = (scope.flags & ALIA_BUTTON_DISABLED) != 0;

    alia_text_style_active(ctx)->color = scope.saved_text_color;
    ctx->geometry->z_base = scope.saved_z_base;

    // Register after children so the button owns the hit (last writer wins).
    alia_element_box_region(
        ctx,
        scope.id,
        &scope.box,
        is_disabled ? ALIA_CURSOR_DEFAULT : ALIA_CURSOR_POINTER,
        ALIA_HIT_TEST_MOUSE);
}

ALIA_EXTERN_C_END
