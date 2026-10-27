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

struct toggle_switch_bit_layout
{
    click_flare_bit_layout click_flare;
    impl::transition_bitfield state_smoothing;
};

struct toggle_switch_data
{
    bitpack<toggle_switch_bit_layout> bits;
    alia_keyboard_click_state keyboard_click_state_;
};

static inline alia_vec2f
toggle_switch_content_origin(
    alia_ui_context* ctx,
    alia_box placement,
    alia_toggle_switch_style const* s)
{
    alia_vec2f const center = placement.min + placement.size * 0.5f;
    return center
         - alia_vec2f{
             alia_px(ctx, s->layout_width) * 0.5f,
             alia_px(ctx, s->layout_height) * 0.5f};
}

void
render_toggle_switch(
    alia_ui_context* ctx,
    alia_box placement,
    toggle_switch_data& data,
    bool state,
    alia_interaction_status_t interaction_status,
    alia_toggle_switch_style const* style)
{
    alia_palette const* p = alia_ctx_palette(ctx);

    alia_vec2f const toggle_switch_min
        = toggle_switch_content_origin(ctx, placement, style);
    float const layout_w = alia_px(ctx, style->layout_width);
    float const layout_h = alia_px(ctx, style->layout_height);
    float const track_w = alia_px(ctx, style->track_width);
    float const track_h = alia_px(ctx, style->track_height);
    float const track_corner = alia_px(
        ctx, style->track_corner_radius_fraction * style->track_height);
    float const dot_y = toggle_switch_min.y + layout_h * 0.5f;

    static alia_animated_transition const transition
        = {alia_default_curve, milliseconds(200)};
    float toggle_position = alia_transition_float(
        alia_ctx_kernel(ctx),
        &transition,
        ALIA_BITREF(data.bits, state_smoothing),
        state,
        1.f,
        0.f);

    float const dot_radius_logical = alia_lerp(
        style->dot_radius_off, style->dot_radius_on, toggle_position);
    float dot_radius = alia_px(ctx, dot_radius_logical);
    float const dot_x_logical = alia_lerp(
        style->dot_center_x_off, style->dot_center_x_on, toggle_position);

    alia_srgba8 const off_dot_rgba
        = alia_responsive_color_resolve(p, style->off_dot, interaction_status);
    alia_srgba8 const on_dot_rgba
        = alia_responsive_color_resolve(p, style->on_dot, interaction_status);
    alia_srgb8 const dot_color = alia_lerp_srgb8_via_oklch(
        alia_srgb8_make(off_dot_rgba.r, off_dot_rgba.g, off_dot_rgba.b),
        alia_srgb8_make(on_dot_rgba.r, on_dot_rgba.g, on_dot_rgba.b),
        toggle_position);

    alia_srgba8 const off_track_rgba = alia_responsive_color_resolve(
        p, style->off_track, interaction_status);
    alia_srgba8 const on_track_rgba = alia_responsive_color_resolve(
        p, style->on_track, interaction_status);
    alia_srgb8 const track_color = alia_lerp_srgb8_via_oklch(
        alia_srgb8_make(off_track_rgba.r, off_track_rgba.g, off_track_rgba.b),
        alia_srgb8_make(on_track_rgba.r, on_track_rgba.g, on_track_rgba.b),
        toggle_position);

    alia_vec2f const track_min{
        toggle_switch_min.x + (layout_w - track_w) * 0.5f,
        toggle_switch_min.y + (layout_h - track_h) * 0.5f};
    alia_draw_rounded_box(
        ctx,
        alia_z(ctx, ALIA_Z_PLANE_BACKGROUND),
        alia_box{track_min, alia_vec2f{track_w, track_h}},
        alia_srgba8_from_srgb8(track_color),
        track_corner);

    float const dot_center_x
        = toggle_switch_min.x + alia_px(ctx, dot_x_logical);
    alia_vec2f const dot_center{dot_center_x, dot_y};

    float const drop = alia_px(ctx, 1.f);
    alia_srgba8 const shadow
        = alia_srgba8_from_srgb8_alpha(dot_color, p->glow_alpha);
    alia_draw_circle(
        ctx,
        alia_z(ctx, ALIA_Z_PLANE_BACKGROUND),
        alia_vec2f{dot_center.x + drop, dot_center.y + drop},
        dot_radius,
        shadow,
        alia_px(ctx, 2.f));

    alia_draw_circle(
        ctx,
        alia_z(ctx, ALIA_Z_PLANE_CONTENT),
        dot_center,
        dot_radius,
        alia_srgba8_from_srgb8(dot_color));

    if ((interaction_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_draw_circle(
            ctx,
            alia_z(ctx, ALIA_Z_PLANE_ACCENT),
            dot_center,
            alia_px(ctx, style->highlight_radius),
            alia_palette_highlight_resolve(p, style->highlight));
    }

    render_click_flares(
        ctx,
        ALIA_NESTED_BITPACK(data.bits, click_flare),
        interaction_status,
        dot_center,
        dot_color,
        alia_px(ctx, style->flare_radius));
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_toggle_switch_style_generate(
    alia_toggle_switch_style* out, alia_style_seeds const* seeds)
{
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    *out = alia_toggle_switch_style{
        .off_track = alia_responsive_color_faded(
            ALIA_PALETTE_COLOR(foundation.structural.weaker_3, 0xff)),
        .on_track = alia_responsive_color_faded(
            ALIA_PALETTE_COLOR(foundation.structural.weaker_3, 0xff)),
        .off_dot = alia_responsive_color_faded(
            ALIA_PALETTE_COLOR(foundation.structural.stronger_2, 0xff)),
        .on_dot = alia_responsive_color_faded(
            ALIA_PALETTE_COLOR(primary.outline, 0xff)),
        .highlight
        = ALIA_PALETTE_COLOR(primary.outline, ALIA_HIGHLIGHT_WEIGHT_ONE),
        // The knob is larger than the track and hangs past its ends.
        .layout_width = 52.f * s.scale,
        .layout_height = 30.f * s.scale,
        .track_width = 26.f * s.scale,
        .track_height = 12.f * s.scale,
        .track_corner_radius_fraction = 0.5f * s.roundness,
        .dot_center_x_off = 16.f * s.scale,
        .dot_center_x_on = 39.f * s.scale,
        .dot_radius_off = 10.f * s.scale,
        .dot_radius_on = 11.f * s.scale,
        .highlight_radius = 17.f * s.scale,
        .flare_radius = 17.f * s.scale,
    };
}

alia_element_id
alia_toggle_switch(
    alia_ui_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags)
{
    // TODO: Use C++ API for this.
    alia_substrate_usage_result result = alia_substrate_use_memory(
        alia_ctx_kernel(ctx),
        sizeof(toggle_switch_data),
        alignof(toggle_switch_data));
    toggle_switch_data* data = (toggle_switch_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) toggle_switch_data{
            .bits = {0},
            .keyboard_click_state_ = {0},
        };
    }
    alia_element_id const id
        = alia_make_element_id(alia_ctx_kernel(ctx), result);

    alia_toggle_switch_style const* const style
        = alia_toggle_switch_style_active(ctx);

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

    bool const is_disabled = ((value->flags & ALIA_SIGNAL_WRITABLE) == 0);

    // TODO: Consider supporting tristate switches.
    bool const selected
        = ((value->flags & ALIA_SIGNAL_READABLE) != 0) ? value->value : false;

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
                // TODO: Fix mouse_button::LEFT
                fire_click_flare(
                    ctx, ALIA_NESTED_BITPACK(data->bits, click_flare));
                // TODO: Use signal utilities here.
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
                        // TODO: Expose click state properly.
                        | (data->keyboard_click_state_.state
                               ? ALIA_INTERACTION_STATUS_ACTIVE
                               : 0));
            render_toggle_switch(
                ctx, box, *data, selected, interaction_status, style);

            break;
        }
    }
    return id;
}

ALIA_EXTERN_C_END
