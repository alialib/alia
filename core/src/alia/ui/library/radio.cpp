#include <alia/abi/ui/library.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/input/keyboard.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/components.h>
#include <alia/abi/ui/palette.h>
#include <alia/impl/events.hpp>
#include <alia/impl/kernel/animation.hpp>
#include <alia/ui/animation.h>

using namespace alia::operators;

namespace alia {

struct radio_bit_layout
{
    click_flare_bit_layout click_flare;
    impl::smoothing_bitfield state_smoothing;
};

struct radio_data
{
    bitpack<radio_bit_layout> bits;
    alia_keyboard_click_state keyboard_click_state_;
};

static alia_radio_style const default_radio_style = {
    .outline = alia_palette_index_foundation_ramp(
        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL, ALIA_PALETTE_RAMP_LEVEL_BASE),
    .dot = alia_palette_index_swatch(
        ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
    .highlight = alia_palette_index_swatch(
        ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
    .layout_width = 40.f,
    .layout_height = 40.f,
    .ring_radius = 12.f,
    .dot_radius = 6.f,
    .highlight_radius = 21.f,
    .border_width = 3.f,
    .flare_radius = 21.f,
};

static inline alia_vec2f
radio_center(alia_box placement)
{
    return placement.min + placement.size * 0.5f;
}

static void
draw_radio_ring(
    alia_context* ctx,
    alia_vec2f center,
    float radius_px,
    float border_width_px,
    alia_rgba border_color)
{
    float const d = radius_px * 2.f;
    alia_box const ring_box{
        alia_vec2f{center.x - radius_px, center.y - radius_px},
        alia_vec2f{d, d},
    };

    alia_box_paint paint{
        .fill_color = alia_rgba{0.f, 0.f, 0.f, 0.f},
        .corner_radius = radius_px,
        .border_width = border_width_px,
        .border_color = border_color,
    };

    alia_draw_box(ctx, ctx->geometry->z_base + 2, ring_box, paint);
}

static void
render_radio(
    alia_context* ctx,
    alia_box placement,
    radio_data& data,
    bool selected,
    alia_interaction_status_t interaction_status,
    alia_radio_style const* style)
{
    alia_palette const* p = alia_ctx_palette(ctx);
    uint8_t const status = static_cast<uint8_t>(interaction_status);

    alia_vec2f const center = radio_center(placement);
    float const ring_radius_px = alia_px(ctx, style->ring_radius);
    float const dot_radius_max_px = alia_px(ctx, style->dot_radius);
    float const border_width_px = alia_px(ctx, style->border_width);

    if (interaction_status & ALIA_INTERACTION_STATUS_DISABLED)
    {
        alia_srgb8 const outline_srgb = alia_palette_color_at(
            p, style->outline, ALIA_INTERACTION_STATUS_DISABLED);
        alia_rgba const outline_rgba
            = alia_rgba_from_rgb(alia_rgb_from_srgb8(outline_srgb));

        draw_radio_ring(
            ctx, center, ring_radius_px, border_width_px, outline_rgba);

        if (selected)
        {
            alia_draw_circle(
                ctx,
                ctx->geometry->z_base + 2,
                center,
                dot_radius_max_px,
                outline_rgba);
        }

        return;
    }

    static alia_animated_transition const transition
        = {alia_default_curve, milliseconds(200)};
    float const smoothed_state = alia_smooth_float(
        ctx,
        &transition,
        ALIA_BITREF(data.bits, state_smoothing),
        selected,
        1.f,
        0.f);

    alia_srgb8 const outline_srgb
        = alia_palette_color_at(p, style->outline, status);
    alia_srgb8 const dot_srgb = alia_palette_color_at(p, style->dot, status);
    alia_srgb8 const highlight_srgb
        = alia_palette_color_at(p, style->highlight, status);

    alia_rgba const outline_rgba
        = alia_rgba_from_rgb(alia_rgb_from_srgb8(outline_srgb));
    alia_rgba const dot_rgba
        = alia_rgba_from_rgb(alia_rgb_from_srgb8(dot_srgb));
    alia_rgba const highlight_rgba
        = alia_rgba_from_rgb_alpha(alia_rgb_from_srgb8(highlight_srgb), 0.2f);

    if ((interaction_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 2,
            center,
            alia_px(ctx, style->highlight_radius),
            highlight_rgba);
    }

    render_click_flares(
        ctx,
        ALIA_NESTED_BITPACK(data.bits, click_flare),
        interaction_status,
        center,
        alia_rgb_from_srgb8(dot_srgb),
        alia_px(ctx, style->flare_radius));

    draw_radio_ring(
        ctx, center, ring_radius_px, border_width_px, outline_rgba);

    float const dot_radius_px = dot_radius_max_px * smoothed_state;
    if (dot_radius_px > 0.f)
    {
        alia_draw_circle(
            ctx, ctx->geometry->z_base + 2, center, dot_radius_px, dot_rgba);
    }
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_element_id
alia_do_radio(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags,
    alia_radio_style const* style)
{
    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(radio_data), alignof(radio_data));
    radio_data* data = (radio_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) radio_data{
            .bits = {0},
            .keyboard_click_state_ = {0},
        };
    }
    alia_element_id const id = result.ptr;

    alia_radio_style const* const effective_style
        = style != nullptr ? style : &default_radio_style;

    bool const is_disabled
        = (value == nullptr) || ((value->flags & ALIA_SIGNAL_WRITABLE) == 0);
    bool const selected
        = (value != nullptr && (value->flags & ALIA_SIGNAL_READABLE) != 0)
            ? value->value
            : false;

    switch (get_event_category(*ctx))
    {
        case ALIA_CATEGORY_REFRESH:
            alia_layout_leaf_emit(
                ctx,
                alia_vec2f{
                    alia_px(ctx, effective_style->layout_width),
                    alia_px(ctx, effective_style->layout_height)},
                layout_flags);
            break;

        case ALIA_CATEGORY_SPATIAL: {
            alia_box box = alia_layout_leaf_read(ctx);
            alia_element_box_region(ctx, id, &box, ALIA_CURSOR_DEFAULT);
            break;
        }

        case ALIA_CATEGORY_INPUT: {
            alia_box box = alia_layout_leaf_read(ctx);
            (void) box;

            if (is_disabled)
                break;

            // TODO: Implement focus order.
            // alia_element_add_to_focus_order(ctx, id);

            if (alia_element_detect_click(ctx, id, ALIA_BUTTON_LEFT)
                // TODO: Implement space bar click.
                /*|| alia_element_detect_space_bar_click(
                    ctx, &data->keyboard_click_state_, id)*/)
            {
                fire_click_flare(
                    ctx, ALIA_NESTED_BITPACK(data->bits, click_flare));

                if (value != nullptr
                    && (value->flags & ALIA_SIGNAL_WRITABLE) != 0)
                {
                    value->value = true;
                    value->flags |= ALIA_SIGNAL_WRITTEN;
                }
            }

            break;
        }

        case ALIA_CATEGORY_DRAWING: {
            alia_box box = alia_layout_leaf_read(ctx);
            alia_interaction_status_t interaction_status
                = alia_element_get_interaction_status(
                    ctx,
                    id,
                    (is_disabled ? ALIA_INTERACTION_STATUS_DISABLED : 0)
                        // TODO: Expose click state properly.
                        | (data->keyboard_click_state_.state
                               ? ALIA_INTERACTION_STATUS_ACTIVE
                               : 0));
            render_radio(
                ctx,
                box,
                *data,
                selected,
                interaction_status,
                effective_style);
            break;
        }

        default:
            alia_layout_leaf_read(ctx);
            break;
    }

    return id;
}

alia_radio_style const*
alia_default_radio_style(void)
{
    return &default_radio_style;
}

ALIA_EXTERN_C_END
