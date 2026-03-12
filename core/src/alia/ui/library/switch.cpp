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

#include <iostream>

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

struct switch_style_info
{
    // track color when switch is disabled
    alia_srgb8 disabled_track_color;
    // dot color when switch is disabled
    alia_srgb8 disabled_dot_color;
    // track color when switch is off
    alia_srgb8 off_track_color;
    // track color when switch is on
    alia_srgb8 on_track_color;
    // dot color when switch is off
    alia_srgb8 off_dot_color;
    // dot color when switch is on
    alia_srgb8 on_dot_color;
    // color of hover/press highlight
    alia_srgb8 highlight_color;
};

switch_style_info
extract_switch_style_info(alia_context* ctx)
{
    alia_palette const* p = alia_ctx_palette(ctx);
    return {
        .disabled_track_color = alia_lerp_srgb8_via_oklch(
            p->foundation.background.base.idle,
            p->foundation.structural.base.idle,
            0.5f),
        .disabled_dot_color = alia_lerp_srgb8_via_oklch(
            p->foundation.background.base.idle,
            p->foundation.structural.base.idle,
            0.6f),
        .off_track_color = p->foundation.structural.weaker_2.idle,
        .on_track_color = p->foundation.structural.weaker_1.idle,
        .off_dot_color = p->foundation.structural.stronger_2.idle,
        .on_dot_color = p->primary.outline.idle,
        .highlight_color = p->primary.outline.idle,
    };
}

void
render_switch(
    alia_context* ctx,
    alia_box placement,
    switch_data& data,
    bool state,
    alia_interaction_status_t interaction_status,
    switch_style_info const& style)
{
    alia_vec2f const center = placement.min + placement.size * 0.5;

    if (interaction_status & ALIA_INTERACTION_STATUS_DISABLED)
    {
        {
            alia_draw_rounded_box(
                ctx,
                ctx->geometry->z_base + 2, // TODO: Proper z offset.
                alia_box{
                    center
                        - alia_vec2f{placement.size.x * 0.25f, alia_px(ctx, 7)},
                    alia_vec2f{placement.size.x * 0.5f, alia_px(ctx, 14)}},
                alia_rgba_from_rgb_alpha(
                    alia_rgb_from_srgb8(style.disabled_track_color), 1.f),
                alia_px(ctx, 7));
        }

        {
            float const dot_x_offset = state ? 0.75f : 0.25f;

            float dot_radius = state ? alia_px(ctx, 11) : alia_px(ctx, 13);

            alia_draw_circle(
                ctx,
                ctx->geometry->z_base + 2, // TODO: Proper z offset.
                {placement.min.x + dot_x_offset * placement.size.x, center.y},
                dot_radius,
                alia_rgba_from_rgb_alpha(
                    alia_rgb_from_srgb8(style.disabled_dot_color), 1.f));
        }

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

    float dot_radius = alia_px(ctx, alia_lerp(12, 13, switch_position));

    float const dot_x_offset = alia_lerp(0.25f, 0.75f, switch_position);

    alia_srgb8 const dot_color = alia_lerp_srgb8_via_oklch(
        style.off_dot_color, style.on_dot_color, switch_position);

    alia_srgb8 const track_color = alia_lerp_srgb8_via_oklch(
        style.off_track_color, style.on_track_color, switch_position);

    alia_draw_rounded_box(
        ctx,
        ctx->geometry->z_base + 2, // TODO: Proper z offset.
        alia_box{
            center - alia_vec2f{placement.size.x * 0.25f, alia_px(ctx, 7)},
            alia_vec2f{placement.size.x * 0.5f, alia_px(ctx, 14)}},
        alia_rgba_from_rgb_alpha(alia_rgb_from_srgb8(track_color), 1.f),
        alia_px(ctx, 7));

    // TODO: Add blur.
    // float const blur_sigma = 4.0f;
    // float const x_drop = 2.0f;
    // float const y_drop = 2.0f;
    // alia_draw_blurred_box(
    //     ctx,
    //     ctx->geometry->z_base + 2, // TODO: Proper z offset.
    //     {placement.min.x + dot_x_offset * placement.size.x + x_drop,
    //      center.y + y_drop},
    //     dot_radius,
    //     alia_rgba_from_rgb_alpha(alia_rgb_from_srgb8(dot_color), 0.8f),
    //     blur_sigma);

    alia_draw_circle(
        ctx,
        ctx->geometry->z_base + 2, // TODO: Proper z offset.
        {placement.min.x + dot_x_offset * placement.size.x, center.y},
        dot_radius,
        alia_rgba_from_rgb_alpha(alia_rgb_from_srgb8(dot_color), 1.0f));

    if ((interaction_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 2, // TODO: Proper z offset.
            {placement.min.x + dot_x_offset * placement.size.x, center.y},
            alia_px(ctx, 20),
            alia_rgba_from_rgb_alpha(
                alia_rgb_from_srgb8(style.highlight_color), 0.2f));
    }

    render_click_flares(
        ctx,
        ALIA_NESTED_BITPACK(data.bits, click_flare),
        interaction_status,
        {placement.min.x + dot_x_offset * placement.size.x, center.y},
        alia_rgb_from_srgb8(dot_color),
        alia_px(ctx, 20));
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_element_id
alia_do_switch(
    alia_context* ctx,
    bool* state, // TODO: Use `alia_signal_bool` instead.
    alia_layout_flags_t layout_flags)
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

    // TODO: Incorporate signal state into this.
    bool const is_disabled = false; // !alia_signal_ready_to_write(state);

    switch (get_event_category(*ctx))
    {
        case ALIA_CATEGORY_REFRESH:
            // TODO: Incorporate baseline logic somehow.
            // TODO: Default alignment?
            alia_layout_leaf_emit(
                ctx,
                alia_vec2f{alia_px(ctx, 55), alia_px(ctx, 30)},
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
                // TODO: Use signals here.
                // if (signal_has_value(state) && signal_ready_to_write(state))
                // {
                //     write_signal(state, !read_signal(state));
                // }
                *state = !*state;
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
            switch_style_info style = extract_switch_style_info(ctx);
            render_switch(ctx, box, *data, *state, interaction_status, style);
        }
    }
    return id;
}

ALIA_EXTERN_C_END
