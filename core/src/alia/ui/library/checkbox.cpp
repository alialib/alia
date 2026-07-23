#include <alia/abi/ui/library.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/input/keyboard.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/palette.h>
#include <alia/impl/events.hpp>
#include <alia/impl/kernel/animation.hpp>
#include <alia/ui/animation.h>
#include <alia/ui/system/object.h>

#if defined(ALIA_HAS_STOCK_FONTS)
#include "alia_fonts.h"
#endif

using namespace alia::operators;

namespace alia {

struct checkbox_bit_layout
{
    click_flare_bit_layout click_flare;
    impl::smoothing_bitfield state_smoothing;
};

struct checkbox_data
{
    bitpack<checkbox_bit_layout> bits;
    alia_keyboard_click_state keyboard_click_state_;
};

static inline alia_vec2f
checkbox_center(alia_box placement)
{
    return placement.min + placement.size * 0.5f;
}

static inline alia_box
checkbox_box(
    alia_context* ctx, alia_box placement, alia_checkbox_style const* style)
{
    alia_vec2f const center = checkbox_center(placement);
    float const side = alia_px(ctx, style->box_size);
    return alia_box{
        alia_vec2f{center.x - side * 0.5f, center.y - side * 0.5f},
        alia_vec2f{side, side}};
}

static void
draw_checkbox_checkmark(
    alia_context* ctx,
    alia_box box,
    alia_srgba8 color,
    alia_checkbox_style const* style)
{
    if (style->checkmark_codepoint == 0 || !ctx->system->msdf_text_engine)
        return;

    float const icon_scale = alia_px(ctx, style->checkmark_size);
    float const icon_width = alia_msdf_measure_codepoint_width(
        ctx->system->msdf_text_engine,
        style->checkmark_font_index,
        style->checkmark_codepoint,
        icon_scale);
    alia_msdf_font_metrics const* icon_metrics = alia_msdf_get_font_metrics(
        ctx->system->msdf_text_engine, style->checkmark_font_index);
    float const icon_height = icon_metrics->line_height * icon_scale;
    alia_vec2f const origin{
        box.min.x + (box.size.x - icon_width) * 0.5f,
        box.min.y + (box.size.y - icon_height) * 0.5f};
    alia_msdf_draw_codepoint(
        ctx->system->msdf_text_engine,
        ctx,
        ctx->geometry->z_base + 3,
        style->checkmark_codepoint,
        icon_scale,
        origin,
        color,
        style->checkmark_font_index);
}

static void
render_checkbox(
    alia_context* ctx,
    alia_box placement,
    checkbox_data& data,
    bool checked,
    alia_interaction_status_t interaction_status,
    alia_checkbox_style const* style)
{
    alia_palette const* p = alia_ctx_palette(ctx);
    alia_vec2f const center = checkbox_center(placement);
    alia_box const box = checkbox_box(ctx, placement, style);

    bool const disabled
        = (interaction_status & ALIA_INTERACTION_STATUS_DISABLED) != 0;
    alia_checkbox_state_style const* state_style
        = disabled
            ? (checked ? &style->disabled_checked : &style->disabled_unchecked)
            : (checked ? &style->checked : &style->unchecked);
    alia_srgba8 const outline
        = alia_palette_color_resolve(p, state_style->outline);
    alia_srgba8 const fill = alia_palette_color_resolve(p, state_style->fill);

    alia_draw_box(
        ctx,
        ctx->geometry->z_base + 2,
        box,
        {.fill_color = fill,
         .corner_radius = alia_px(ctx, style->box_corner_radius),
         .border_width = alia_px(ctx, style->border_width),
         .border_color = outline});

    if (!disabled)
    {
        alia_srgb8 const flare_srgb
            = alia_palette_srgb_at(p, style->highlight.index);
        render_click_flares(
            ctx,
            ALIA_NESTED_BITPACK(data.bits, click_flare),
            interaction_status,
            center,
            flare_srgb,
            alia_px(ctx, style->flare_radius));
    }

    static alia_animated_transition const transition
        = {alia_default_curve, milliseconds(160)};
    float checkmark_opacity = checked ? 1.f : 0.f;

    if (checkmark_opacity > 0.f)
    {
        alia_srgba8 checkmark_color = alia_palette_color_resolve(
            p, disabled ? style->disabled_checkmark : style->checkmark);
        checkmark_color.a
            = uint8_t(float(checkmark_color.a) * checkmark_opacity);
        draw_checkbox_checkmark(ctx, box, checkmark_color, style);
    }

    if ((interaction_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 2,
            center,
            alia_px(ctx, style->highlight_radius),
            alia_palette_color_resolve(p, style->highlight));
    }
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_checkbox_style_generate(
    alia_checkbox_style* out, alia_style_seeds const* seeds)
{
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    *out = alia_checkbox_style{
        .unchecked =
            {
                .outline = alia_palette_color_make(
                    alia_palette_index_foundation_ramp(
                        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
                        ALIA_PALETTE_RAMP_LEVEL_BASE),
                    0xff),
                .fill = alia_palette_color_make(
                    alia_palette_index_foundation_ramp(
                        ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
                        ALIA_PALETTE_RAMP_LEVEL_BASE),
                    0x00),
            },
        .checked =
            {
                .outline = alia_palette_color_make(
                    alia_palette_index_swatch(
                        ALIA_PALETTE_SWATCH_PRIMARY,
                        ALIA_PALETTE_SWATCH_PART_SOLID),
                    0xff),
                .fill = alia_palette_color_make(
                    alia_palette_index_swatch(
                        ALIA_PALETTE_SWATCH_PRIMARY,
                        ALIA_PALETTE_SWATCH_PART_SOLID),
                    0xff),
            },
        .disabled_unchecked =
            {
                .outline = alia_palette_color_make(
                    alia_palette_index_foundation_ramp(
                        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
                        ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
                    0xa0),
                .fill = alia_palette_color_make(
                    alia_palette_index_foundation_ramp(
                        ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
                        ALIA_PALETTE_RAMP_LEVEL_BASE),
                    0x00),
            },
        .disabled_checked =
            {
                .outline = alia_palette_color_make(
                    alia_palette_index_foundation_ramp(
                        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
                        ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
                    0xa0),
                .fill = alia_palette_color_make(
                    alia_palette_index_foundation_ramp(
                        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
                        ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
                    0xa0),
            },

        .checkmark = alia_palette_color_make(
            alia_palette_index_foundation_ramp(
                ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
                ALIA_PALETTE_RAMP_LEVEL_BASE),
            0xff),
        .disabled_checkmark = alia_palette_color_make(
            alia_palette_index_foundation_ramp(
                ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
                ALIA_PALETTE_RAMP_LEVEL_BASE),
            0xff),

        .highlight = alia_palette_color_make(
            alia_palette_index_swatch(
                ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
            0x18),

        .layout_width = 40.f * s.scale,
        .layout_height = 40.f * s.scale,
        .box_size = 22.f * s.scale,
        .box_corner_radius = 5.f * s.scale * s.roundness,
        .border_width = 3.f * s.scale,
        .checkmark_size = 18.f * s.scale,
#if defined(ALIA_HAS_STOCK_FONTS)
        .checkmark_font_index = alia_font_material_symbols_outlined_index,
        .checkmark_codepoint = alia_font_material_symbols_outlined_icon_check,
#else
        .checkmark_font_index = 0,
        .checkmark_codepoint = 0,
#endif
        .highlight_radius = 21.f * s.scale,
        .flare_radius = 21.f * s.scale,
    };
}

alia_element_id
alia_do_checkbox(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags)
{
    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(checkbox_data), alignof(checkbox_data));
    checkbox_data* data = (checkbox_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) checkbox_data{
            .bits = {0},
            .keyboard_click_state_ = {0},
        };
    }
    alia_element_id const id = alia_make_element_id(ctx, result);

    alia_checkbox_style const* const style = alia_checkbox_style_active(ctx);

    alia_event_category const category = get_event_category(*ctx);
    if (category == ALIA_CATEGORY_REFRESH)
    {
        alia_layout_leaf_emit(
            ctx,
            alia_layout_content_metrics_make(
                alia_vec2f{
                    alia_px(ctx, style->layout_width),
                    alia_px(ctx, style->layout_height)}),
            layout_flags);
        return id;
    }

    alia_box box = alia_layout_consume_box(ctx);

    bool const is_disabled
        = (value == nullptr) || ((value->flags & ALIA_SIGNAL_WRITABLE) == 0);
    bool const checked
        = (value != nullptr && (value->flags & ALIA_SIGNAL_READABLE) != 0)
            ? value->value
            : false;

    switch (category)
    {
        case ALIA_CATEGORY_SPATIAL: {
            alia_element_box_region(
                ctx, id, &box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
            break;
        }

        case ALIA_CATEGORY_INPUT: {
            if (is_disabled)
                break;

            if (alia_element_detect_click(ctx, id, ALIA_BUTTON_LEFT))
            {
                fire_click_flare(
                    ctx, ALIA_NESTED_BITPACK(data->bits, click_flare));
                value->value = !value->value;
                value->flags |= ALIA_SIGNAL_WRITTEN;
            }
            break;
        }

        case ALIA_CATEGORY_DRAWING: {
            alia_interaction_status_t interaction_status
                = alia_element_get_interaction_status(
                    ctx,
                    id,
                    (is_disabled ? ALIA_INTERACTION_STATUS_DISABLED : 0)
                        | (data->keyboard_click_state_.state
                               ? ALIA_INTERACTION_STATUS_ACTIVE
                               : 0));
            render_checkbox(
                ctx, box, *data, checked, interaction_status, style);
            break;
        }
    }

    return id;
}

ALIA_EXTERN_C_END
