#include <alia/abi/ui/library.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
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

struct switch_bit_layout
{
    click_flare_bit_layout click_flare;
    impl::smoothing_bitfield state_smoothing;
};

struct switch_data
{
    bitpack<switch_bit_layout> bits;
    alia_keyboard_click_state keyboard_click_state_;
};

static alia_switch_style const default_switch_style = {
    .off_track = alia_palette_index_foundation_ramp(
        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
        ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
    .on_track = alia_palette_index_foundation_ramp(
        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
        ALIA_PALETTE_RAMP_LEVEL_WEAKER_1),
    .off_dot = alia_palette_index_foundation_ramp(
        ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
        ALIA_PALETTE_RAMP_LEVEL_STRONGER_2),
    .on_dot = alia_palette_index_swatch(
        ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
    .highlight = alia_palette_index_swatch(
        ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
    .layout_width = 55.f,
    .layout_height = 30.f,
    .track_width = 27.5f,
    .track_height = 12.f,
    .track_corner_radius_fraction = 0.5f,
    .dot_center_x_off = 13.75f,
    .dot_center_x_on = 41.25f,
    .dot_radius_off = 11.f,
    .dot_radius_on = 13.f,
    .highlight_radius = 20.f,
    .flare_radius = 20.f,
};

static inline alia_vec2f
switch_content_origin(
    alia_context* ctx, alia_box placement, alia_switch_style const* s)
{
    alia_vec2f const center = placement.min + placement.size * 0.5f;
    return center
         - alia_vec2f{
             alia_px(ctx, s->layout_width) * 0.5f,
             alia_px(ctx, s->layout_height) * 0.5f};
}

void
render_switch(
    alia_context* ctx,
    alia_box placement,
    switch_data& data,
    bool state,
    alia_interaction_status_t interaction_status,
    alia_switch_style const* style)
{
    alia_palette const* p = alia_ctx_palette(ctx);
    uint8_t const status
        = (uint8_t) interaction_status; // for palette_color_at

    alia_vec2f const switch_min = switch_content_origin(ctx, placement, style);
    float const layout_w = alia_px(ctx, style->layout_width);
    float const layout_h = alia_px(ctx, style->layout_height);
    float const track_w = alia_px(ctx, style->track_width);
    float const track_h = alia_px(ctx, style->track_height);
    float const track_corner = alia_px(
        ctx, style->track_corner_radius_fraction * style->track_height);
    float const dot_y = switch_min.y + layout_h * 0.5f;

    if (interaction_status & ALIA_INTERACTION_STATUS_DISABLED)
    {
        alia_srgb8 const track_color = alia_palette_color_at(
            p, style->off_track, ALIA_INTERACTION_STATUS_DISABLED);
        alia_srgb8 const dot_color = alia_palette_color_at(
            p, style->off_dot, ALIA_INTERACTION_STATUS_DISABLED);

        alia_vec2f const track_min{
            switch_min.x + (layout_w - track_w) * 0.5f,
            switch_min.y + (layout_h - track_h) * 0.5f};
        alia_draw_rounded_box(
            ctx,
            ctx->geometry->z_base + 2, // TODO: Proper z offset.
            alia_box{track_min, alia_vec2f{track_w, track_h}},
            alia_srgba8_from_srgb8(track_color),
            track_corner);

        float const dot_x_logical
            = state ? style->dot_center_x_on : style->dot_center_x_off;
        /* Same radii as animated end states: off -> dot_radius_off, on ->
         * dot_radius_on */
        float const dot_radius_logical
            = state ? style->dot_radius_on : style->dot_radius_off;
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 2, // TODO: Proper z offset.
            {switch_min.x + alia_px(ctx, dot_x_logical), dot_y},
            alia_px(ctx, dot_radius_logical),
            alia_srgba8_from_srgb8(dot_color));

        return;
    }

    static alia_animated_transition const transition
        = {alia_default_curve, milliseconds(200)};
    float switch_position = alia_smooth_float(
        ctx,
        &transition,
        ALIA_BITREF(data.bits, state_smoothing),
        state,
        1.f,
        0.f);

    float const dot_radius_logical = alia_lerp(
        style->dot_radius_off, style->dot_radius_on, switch_position);
    float dot_radius = alia_px(ctx, dot_radius_logical);
    float const dot_x_logical = alia_lerp(
        style->dot_center_x_off, style->dot_center_x_on, switch_position);

    alia_srgb8 const off_dot
        = alia_palette_color_at(p, style->off_dot, status);
    alia_srgb8 const on_dot = alia_palette_color_at(p, style->on_dot, status);
    alia_srgb8 const dot_color
        = alia_lerp_srgb8_via_oklch(off_dot, on_dot, switch_position);

    alia_srgb8 const off_track
        = alia_palette_color_at(p, style->off_track, status);
    alia_srgb8 const on_track
        = alia_palette_color_at(p, style->on_track, status);
    alia_srgb8 const track_color
        = alia_lerp_srgb8_via_oklch(off_track, on_track, switch_position);

    alia_vec2f const track_min{
        switch_min.x + (layout_w - track_w) * 0.5f,
        switch_min.y + (layout_h - track_h) * 0.5f};
    alia_draw_rounded_box(
        ctx,
        ctx->geometry->z_base + 2, // TODO: Proper z offset.
        alia_box{track_min, alia_vec2f{track_w, track_h}},
        alia_srgba8_from_srgb8(track_color),
        track_corner);

    float const dot_center_x = switch_min.x + alia_px(ctx, dot_x_logical);
    alia_vec2f const dot_center{dot_center_x, dot_y};

    // TODO: Add blur.
    // float const blur_sigma = 4.0f;
    // float const x_drop = 2.0f;
    // float const y_drop = 2.0f;
    // alia_draw_blurred_box(
    //     ctx,
    //     ctx->geometry->z_base + 2, // TODO: Proper z offset.
    //     {dot_center_x + x_drop,
    //      dot_y + y_drop},
    //     dot_radius,
    //     alia_rgba_from_rgb_alpha(alia_rgb_from_srgb8(dot_color), 0.8f),
    //     blur_sigma);

    alia_draw_circle(
        ctx,
        ctx->geometry->z_base + 2, // TODO: Proper z offset.
        dot_center,
        dot_radius,
        alia_srgba8_from_srgb8(dot_color));

    if ((interaction_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_srgb8 const highlight_color
            = alia_palette_color_at(p, style->highlight, status);
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 2, // TODO: Proper z offset.
            dot_center,
            alia_px(ctx, style->highlight_radius),
            alia_srgba8_from_srgb8_alpha(highlight_color, 0x33));
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

alia_element_id
alia_do_switch(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags,
    alia_switch_style const* style)
{
    // TODO: Use C++ API for this.
    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(switch_data), alignof(switch_data));
    switch_data* data = (switch_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) switch_data{
            .bits = {0},
            .keyboard_click_state_ = {0},
        };
    }
    auto const id = result.ptr;

    bool const is_disabled = ((value->flags & ALIA_SIGNAL_WRITABLE) == 0);

    // TODO: Consider supporting tristate switches.
    bool const selected
        = ((value->flags & ALIA_SIGNAL_READABLE) != 0) ? value->value : false;

    alia_switch_style const* const effective_style
        = style != nullptr ? style : &default_switch_style;

    switch (get_event_category(*ctx))
    {
        case ALIA_CATEGORY_REFRESH:
            // TODO: Incorporate baseline logic somehow.
            // TODO: Default alignment?
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
                // TODO
                // abort_traversal(*ctx);
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
            render_switch(
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

alia_switch_style const*
alia_default_switch_style(void)
{
    return &default_switch_style;
}

ALIA_EXTERN_C_END
