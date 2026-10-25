#include <alia/abi/ui/library.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/input/keyboard.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/palette.h>
#include <alia/impl/events.hpp>
#include <alia/impl/kernel/animation.hpp>
#include <alia/ui/animation.h>

using namespace alia::operators;

namespace alia {

struct radio_button_bit_layout
{
    click_flare_bit_layout click_flare;
    impl::transition_bitfield state_smoothing;
};

struct radio_button_data
{
    bitpack<radio_button_bit_layout> bits;
    alia_keyboard_click_state keyboard_click_state_;
};

static inline alia_vec2f
radio_button_center(alia_box placement)
{
    return placement.min + placement.size * 0.5f;
}

static void
draw_radio_button_ring(
    alia_ui_context* ctx,
    alia_vec2f center,
    float radius_px,
    float border_width_px,
    alia_srgb8 border_color)
{
    float const d = radius_px * 2.f;
    alia_box const ring_box{
        alia_vec2f{center.x - radius_px, center.y - radius_px},
        alia_vec2f{d, d},
    };

    alia_box_paint paint{
        .fill_color = alia_srgba8{0, 0, 0, 0},
        .corner_radius = radius_px,
        .border_width = border_width_px,
        .border_color = alia_srgba8_from_srgb8(border_color),
    };

    alia_draw_box(ctx, alia_z(ctx, ALIA_Z_PLANE_BACKGROUND), ring_box, paint);
}

static void
render_radio_button(
    alia_ui_context* ctx,
    alia_box placement,
    radio_button_data& data,
    bool selected,
    alia_interaction_status_t interaction_status,
    alia_radio_button_style const* style)
{
    alia_palette const* p = alia_ctx_palette(ctx);

    alia_vec2f const center = radio_button_center(placement);
    float const ring_radius_px = alia_px(ctx, style->ring_radius);
    float const dot_radius_max_px = alia_px(ctx, style->dot_radius);
    float const border_width_px = alia_px(ctx, style->border_width);

    bool const is_disabled
        = (interaction_status & ALIA_INTERACTION_STATUS_DISABLED) != 0;

    static alia_animated_transition const transition
        = {alia_default_curve, milliseconds(200)};
    float const smoothed_state = alia_transition_float(
        alia_ctx_kernel(ctx),
        &transition,
        ALIA_BITREF(data.bits, state_smoothing),
        selected,
        1.f,
        0.f);

    alia_srgba8 const outline_rgba
        = alia_responsive_color_resolve(p, style->outline, interaction_status);
    alia_srgba8 const dot_rgba
        = alia_responsive_color_resolve(p, style->dot, interaction_status);
    alia_srgb8 const outline_srgb
        = alia_srgb8_make(outline_rgba.r, outline_rgba.g, outline_rgba.b);
    alia_srgb8 const dot_srgb
        = alia_srgb8_make(dot_rgba.r, dot_rgba.g, dot_rgba.b);
    // The ring joins the dot as the control becomes selected.
    alia_srgb8 const ring_srgb
        = alia_lerp_srgb8_via_oklch(outline_srgb, dot_srgb, smoothed_state);

    if (!is_disabled
        && (interaction_status
            & (ALIA_INTERACTION_STATUS_ACTIVE
               | ALIA_INTERACTION_STATUS_HOVERED))
               != 0)
    {
        alia_draw_circle(
            ctx,
            alia_z(ctx, ALIA_Z_PLANE_BACKGROUND),
            center,
            alia_px(ctx, style->highlight_radius),
            alia_palette_color_resolve(p, style->highlight));
    }

    if (!is_disabled)
    {
        render_click_flares(
            ctx,
            ALIA_NESTED_BITPACK(data.bits, click_flare),
            interaction_status,
            center,
            dot_srgb,
            alia_px(ctx, style->flare_radius));
    }

    draw_radio_button_ring(
        ctx, center, ring_radius_px, border_width_px, ring_srgb);

    float const dot_radius_px = dot_radius_max_px * smoothed_state;
    if (dot_radius_px > 0.f)
    {
        alia_draw_circle(
            ctx,
            alia_z(ctx, ALIA_Z_PLANE_CONTENT),
            center,
            dot_radius_px,
            alia_srgba8_from_srgb8(dot_srgb));
    }
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_radio_button_style_generate(
    alia_radio_button_style* out, alia_style_seeds const* seeds)
{
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    *out = alia_radio_button_style{
        .outline = alia_responsive_color_faded(
            ALIA_PALETTE_COLOR(foundation.structural.weaker_2, 0xff)),
        .dot = alia_responsive_color_faded(
            ALIA_PALETTE_COLOR(primary.outline, 0xff)),
        .highlight = ALIA_PALETTE_COLOR(primary.outline, 0x18),
        // Leaf is the ring, with a little room around it. The hover halo
        // paints outside this box.
        .layout_width = 28.f * s.scale,
        .layout_height = 28.f * s.scale,
        .ring_radius = 11.f * s.scale,
        .dot_radius = 5.f * s.scale,
        .highlight_radius = 18.f * s.scale,
        .border_width = 2.f * s.scale,
        .flare_radius = 18.f * s.scale,
    };
}

alia_element_id
alia_radio_button(
    alia_ui_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags)
{
    alia_substrate_usage_result result = alia_substrate_use_memory(
        alia_ctx_kernel(ctx),
        sizeof(radio_button_data),
        alignof(radio_button_data));
    radio_button_data* data = (radio_button_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) radio_button_data{
            .bits = {0},
            .keyboard_click_state_ = {0},
        };
    }
    alia_element_id const id
        = alia_make_element_id(alia_ctx_kernel(ctx), result);

    alia_radio_button_style const* const style
        = alia_radio_button_style_active(ctx);

    alia_event_category const category = get_event_category(*ctx);
    if (category == ALIA_CATEGORY_REFRESH)
    {
        alia_layout_leaf_emit(
            ctx,
            alia_layout_content_metrics_make(
                alia_vec2f{style->layout_width, style->layout_height}),
            layout_flags);
        return id;
    }

    bool const is_disabled
        = (value == nullptr) || ((value->flags & ALIA_SIGNAL_WRITABLE) == 0);
    bool const selected
        = (value != nullptr && (value->flags & ALIA_SIGNAL_READABLE) != 0)
            ? value->value
            : false;

    alia_box const box = alia_layout_consume_box(ctx);

    switch (category)
    {
        case ALIA_CATEGORY_SPATIAL: {
            alia_element_box_region(
                ctx,
                id,
                &box,
                ALIA_Z_PLANE_CONTENT,
                ALIA_HIT_TEST_MOUSE,
                ALIA_CURSOR_DEFAULT);
            break;
        }

        case ALIA_CATEGORY_INPUT: {
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
            alia_interaction_status_t interaction_status
                = alia_element_get_interaction_status(
                    ctx,
                    id,
                    (is_disabled ? ALIA_INTERACTION_STATUS_DISABLED : 0)
                        // TODO: Expose click state properly.
                        | (data->keyboard_click_state_.state
                               ? ALIA_INTERACTION_STATUS_ACTIVE
                               : 0));
            render_radio_button(
                ctx, box, *data, selected, interaction_status, style);
            break;
        }
    }

    return id;
}

ALIA_EXTERN_C_END
