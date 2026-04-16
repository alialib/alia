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

struct node_expander_bit_layout
{
    click_flare_bit_layout click_flare;
    impl::smoothing_bitfield state_smoothing;
};

struct node_expander_data
{
    bitpack<node_expander_bit_layout> bits;
    alia_keyboard_click_state keyboard_click_state_;
};

static alia_node_expander_style const default_node_expander_style = {
    .triangle = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
            ALIA_PALETTE_RAMP_LEVEL_BASE),
        0xff),
    .disabled_triangle = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
            ALIA_PALETTE_RAMP_LEVEL_WEAKER_4),
        0xff),
    .highlight = alia_palette_color_make(
        alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
        0x30),

    .layout_width = 48.f,
    .layout_height = 48.f,

    .triangle_side = 24.f,

    // triangle "right" -> "down" rotation
    .collapsed_rotation_degrees = 90.f,
    .expanded_rotation_degrees = 180.f,

    .highlight_radius = 20.f,
    .flare_radius = 20.f,
};

static inline alia_vec2f
node_expander_center(alia_box placement)
{
    return placement.min + placement.size * 0.5f;
}

static inline alia_box
node_expander_triangle_box(
    alia_context* ctx,
    alia_box placement,
    alia_node_expander_style const* style)
{
    float const side = alia_px(ctx, style->triangle_side);
    alia_vec2f const center = node_expander_center(placement);
    return alia_box{
        alia_vec2f{center.x - side * 0.5f, center.y - side * 0.5f},
        alia_vec2f{side, side}};
}

static void
render_node_expander(
    alia_context* ctx,
    alia_box placement,
    node_expander_data& data,
    bool expanded,
    alia_interaction_status_t interaction_status,
    alia_node_expander_style const* style)
{
    alia_palette const* p = alia_ctx_palette(ctx);

    bool const is_disabled
        = (interaction_status & ALIA_INTERACTION_STATUS_DISABLED) != 0;

    static alia_animated_transition const transition
        = {alia_default_curve, milliseconds(200)};
    float const rotation_degrees = alia_smooth_float(
        ctx,
        &transition,
        ALIA_BITREF(data.bits, state_smoothing),
        expanded,
        style->expanded_rotation_degrees,
        style->collapsed_rotation_degrees);

    alia_box const triangle_box
        = node_expander_triangle_box(ctx, placement, style);
    alia_vec2f const center = node_expander_center(placement);

    alia_srgba8 const triangle_rgba
        = is_disabled ? alia_palette_color_resolve(p, style->disabled_triangle)
                      : alia_palette_color_resolve(p, style->triangle);

    alia_draw_equilateral_triangle(
        ctx,
        ctx->geometry->z_base + 2,
        triangle_box,
        triangle_rgba,
        rotation_degrees);

    if (is_disabled)
        return;

    // highlight circle for hover/active states
    if ((interaction_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 3,
            center,
            alia_px(ctx, style->highlight_radius),
            alia_palette_color_resolve(p, style->highlight));
    }

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

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_element_id
alia_do_node_expander(
    alia_context* ctx,
    alia_bool_signal* expanded,
    alia_layout_flags_t layout_flags,
    alia_node_expander_style const* style)
{
    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(node_expander_data), alignof(node_expander_data));
    node_expander_data* data = (node_expander_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) node_expander_data{
            .bits = {0},
            .keyboard_click_state_ = {0},
        };
    }

    alia_element_id const id = alia_make_element_id(ctx, result);

    bool const is_disabled = (expanded == nullptr)
                          || ((expanded->flags & ALIA_SIGNAL_WRITABLE) == 0);

    alia_node_expander_style const* const effective_style
        = style != nullptr ? style : &default_node_expander_style;

    alia_event_category const category = get_event_category(*ctx);
    if (category == ALIA_CATEGORY_REFRESH)
    {
        alia_layout_leaf_emit(
            ctx,
            alia_vec2f{
                alia_px(ctx, effective_style->layout_width),
                alia_px(ctx, effective_style->layout_height)},
            layout_flags);
        return id;
    }

    alia_box const box = alia_layout_consume_box(ctx);

    // TODO: Consider supporting an indeterminate state.
    bool const expanded_state
        = (expanded != nullptr
           && (expanded->flags & ALIA_SIGNAL_READABLE) != 0)
            ? expanded->value
            : false;

    switch (category)
    {
        case ALIA_CATEGORY_SPATIAL: {
            alia_element_box_region(ctx, id, &box, ALIA_CURSOR_DEFAULT);
            break;
        }

        case ALIA_CATEGORY_INPUT: {
            if (is_disabled)
                break;

            alia_element_add_to_focus_order(ctx, id);

            // TODO: Handle keyboard input.
            if (alia_element_detect_click(ctx, id, ALIA_BUTTON_LEFT))
            {
                fire_click_flare(
                    ctx, ALIA_NESTED_BITPACK(data->bits, click_flare));
                expanded->value = !expanded->value;
                expanded->flags |= ALIA_SIGNAL_WRITTEN;
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
            render_node_expander(
                ctx,
                box,
                *data,
                expanded_state,
                interaction_status,
                effective_style);
            break;
        }
    }

    return id;
}

alia_node_expander_style const*
alia_default_node_expander_style(void)
{
    return &default_node_expander_style;
}

ALIA_EXTERN_C_END
