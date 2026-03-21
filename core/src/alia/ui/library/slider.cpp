#include <alia/abi/ui/library.h>

#include <alia/abi/ids.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/components.h>
#include <alia/abi/ui/palette.h>
#include <alia/impl/events.hpp>

#include <cmath>
#include <cstdint>

using namespace alia::operators;

namespace alia {

struct slider_data
{
    uint32_t reserved;
};

static alia_slider_style const default_slider_style = {
    .track_color = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
            ALIA_PALETTE_RAMP_LEVEL_WEAKER_1),
        0xff),
    .thumb_color = alia_palette_color_make(
        alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
        0xff),
    .highlight = alia_palette_color_make(
        alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_OUTLINE),
        0x33),
    .layout_width = 320.f,
    .layout_height = 32.f,
    .track_thickness = 6.f,
    .thumb_radius = 12.f,
    .highlight_radius = 18.f,
};

static inline float
left_pad(alia_context* ctx, alia_slider_style const* s)
{
    return alia_px(ctx, s->thumb_radius);
}

static inline float
right_pad(alia_context* ctx, alia_slider_style const* s)
{
    return alia_px(ctx, s->thumb_radius);
}

static double
round_and_clamp(double x, double min_v, double max_v, double step)
{
    double clamped = x;
    if (clamped < min_v)
        clamped = min_v;
    else if (clamped > max_v)
        clamped = max_v;
    if (step != 0)
    {
        return std::floor((clamped - min_v) / step + 0.5) * step + min_v;
    }
    return clamped;
}

// Returns (max - min) / usable_track_length_in_pixels.
static double
get_values_per_pixel(
    alia_box const& box,
    unsigned axis,
    alia_context* ctx,
    alia_slider_style const* s,
    double minimum,
    double maximum)
{
    float const usable = (axis ? box.size.y : box.size.x) - left_pad(ctx, s)
                       - right_pad(ctx, s) - 1.f;
    if (usable <= 0.f || maximum == minimum)
        return 0.;
    return (maximum - minimum) / double(usable);
}

static alia_vec2f
get_track_min(
    alia_box const& box,
    unsigned axis,
    alia_context* ctx,
    alia_slider_style const* s)
{
    float const track_w
        = axis == 0 ? (box.size.x - left_pad(ctx, s) - right_pad(ctx, s))
                    : alia_px(ctx, s->track_thickness);
    float const track_h
        = axis == 1 ? (box.size.y - left_pad(ctx, s) - right_pad(ctx, s))
                    : alia_px(ctx, s->track_thickness);
    alia_vec2f min = box.min;
    if (axis == 0)
    {
        min.x += left_pad(ctx, s);
        min.y += (box.size.y - track_h) * 0.5f;
    }
    else
    {
        min.y += left_pad(ctx, s);
        min.x += (box.size.x - track_w) * 0.5f;
    }
    return min;
}

static alia_vec2f
get_track_size(
    alia_box const& box,
    unsigned axis,
    alia_context* ctx,
    alia_slider_style const* s)
{
    float const len
        = axis == 0 ? (box.size.x - left_pad(ctx, s) - right_pad(ctx, s))
                    : (box.size.y - left_pad(ctx, s) - right_pad(ctx, s));
    float const thick = alia_px(ctx, s->track_thickness);
    if (axis == 0)
        return {len, thick};
    return {thick, len};
}

static alia_vec2f
get_thumb_center(
    alia_box const& box,
    unsigned axis,
    alia_context* ctx,
    alia_slider_style const* s,
    double minimum,
    double maximum,
    double value)
{
    double const vpp
        = get_values_per_pixel(box, axis, ctx, s, minimum, maximum);
    float const pad = left_pad(ctx, s);
    alia_vec2f center;
    if (axis == 0)
    {
        center.y = box.min.y + box.size.y * 0.5f;
        center.x = vpp <= 0. ? box.min.x + pad
                             : static_cast<float>(
                                   box.min.x + pad + (value - minimum) / vpp);
    }
    else
    {
        center.x = box.min.x + box.size.x * 0.5f;
        center.y = vpp <= 0. ? box.min.y + pad
                             : static_cast<float>(
                                   box.min.y + pad + (value - minimum) / vpp);
    }
    return center;
}

static alia_box
get_thumb_region(
    alia_box const& box,
    unsigned axis,
    alia_context* ctx,
    alia_slider_style const* s,
    double minimum,
    double maximum,
    double value)
{
    alia_vec2f const c
        = get_thumb_center(box, axis, ctx, s, minimum, maximum, value);
    float const r = alia_px(ctx, s->thumb_radius) * 1.25f;
    return {{c.x - r, c.y - r}, {2 * r, 2 * r}};
}

static void
render_slider(
    alia_context* ctx,
    alia_box box,
    unsigned axis,
    alia_slider_style const* s,
    double minimum,
    double maximum,
    double value,
    alia_interaction_status_t thumb_status)
{
    alia_palette const* p = alia_ctx_palette(ctx);

    alia_vec2f const track_min = get_track_min(box, axis, ctx, s);
    alia_vec2f const track_size = get_track_size(box, axis, ctx, s);
    alia_box const track_box{track_min, track_size};

    // TODO: Should the track use interaction status?
    alia_srgba8 const track_rgba
        = alia_palette_color_resolve(p, s->track_color);

    alia_draw_rounded_box(
        ctx,
        ctx->geometry->z_base + 2,
        track_box,
        track_rgba,
        0.f);

    alia_vec2f const thumb_center
        = get_thumb_center(box, axis, ctx, s, minimum, maximum, value);
    alia_srgb8 const thumb_srgb
        = alia_palette_srgb_at(p, s->thumb_color.index);

    float const thumb_r = alia_px(ctx, s->thumb_radius);

    // TODO: Add blur (legacy Skia blur shadow). Backend not ready.
    // float const blur_sigma = 4.0f;
    // float const x_drop = 2.0f;
    // float const y_drop = 2.0f;
    // alia_draw_blurred_box(...)

    alia_draw_circle(
        ctx,
        ctx->geometry->z_base + 2,
        thumb_center,
        thumb_r,
        alia_srgba8_from_srgb8(thumb_srgb));

    if ((thumb_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_draw_circle(
            ctx,
            ctx->geometry->z_base + 2,
            thumb_center,
            alia_px(ctx, s->highlight_radius),
            alia_palette_color_resolve(p, s->highlight));
    }

    // TODO: Draw focus
    // if (thumb_state & WIDGET_FOCUSED)
    // {
    //     draw_focus_rect(
    //         ctx,
    //         data.focus_rendering,
    //         data.layout_node.assignment().region);
    // }
}

template<typename WriteFn, typename ReadFn>
static alia_element_id
do_slider_impl(
    alia_context* ctx,
    WriteFn write_value,
    ReadFn read_value,
    double minimum,
    double maximum,
    double step,
    alia_layout_flags_t layout_flags,
    bool vertical,
    alia_slider_style const* style)
{
    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(slider_data), alignof(slider_data));
    slider_data* data = reinterpret_cast<slider_data*>(result.ptr);
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) slider_data{.reserved = 0};
    }
    alia_element_id const base_id = result.ptr;
    alia_element_id const track_id = base_id;
    alia_element_id const thumb_id = alia_offset_id(base_id, 1);

    bool const is_disabled = false;

    alia_slider_style const* const effective_style
        = style != nullptr ? style : &default_slider_style;

    unsigned const axis = vertical ? 1u : 0u;

    switch (get_event_category(*ctx))
    {
        case ALIA_CATEGORY_REFRESH:
            alia_layout_leaf_emit(
                ctx,
                {alia_px(ctx, effective_style->layout_width),
                 alia_px(ctx, effective_style->layout_height)},
                layout_flags);
            break;

        case ALIA_CATEGORY_SPATIAL: {
            alia_box box = alia_layout_leaf_read(ctx);
            alia_element_box_region(ctx, track_id, &box, ALIA_CURSOR_DEFAULT);
            double const current = read_value();
            alia_box const thumb_box = get_thumb_region(
                box, axis, ctx, effective_style, minimum, maximum, current);
            alia_element_box_region(
                ctx, thumb_id, &thumb_box, ALIA_CURSOR_DEFAULT);
            break;
        }

        case ALIA_CATEGORY_INPUT: {
            if (is_disabled)
                break;

            alia_box box = alia_layout_leaf_read(ctx);

            // TODO: Implement focus order.
            // alia_element_add_to_focus_order(ctx, thumb_id);

            if (alia_element_detect_press_or_drag(
                    ctx, track_id, ALIA_BUTTON_LEFT)
                || alia_element_detect_press_or_drag(
                    ctx, thumb_id, ALIA_BUTTON_LEFT))
            {
                alia_vec2f const p = alia_input_pointer_position(ctx);
                double const p_axis = axis ? static_cast<double>(p.y)
                                           : static_cast<double>(p.x);
                double const min_axis = axis ? static_cast<double>(box.min.y)
                                             : static_cast<double>(box.min.x);
                double new_value
                    = (p_axis - min_axis
                       - static_cast<double>(left_pad(ctx, effective_style)))
                        * get_values_per_pixel(
                            box, axis, ctx, effective_style, minimum, maximum)
                    + minimum;
                write_value(
                    round_and_clamp(new_value, minimum, maximum, step));
                // TODO: Set focus!!
                // set_focus(ctx, thumb_id);
            }

            // TODO: Handle keyboard input!
            /*
            key_event_info info;
            if (detect_key_press(ctx, &info, thumb_id) && info.mods == 0)
            {
                double increment = (maximum - minimum) / 10;
                switch (info.code)
                {
                    case KEY_LEFT:
                        if (axis == 0)
                        {
                            write_signal(
                                value,
                                round_and_clamp(
                                    get(value) - increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_DOWN:
                        if (axis == 1)
                        {
                            set_new_value(
                                value,
                                result,
                                round_and_clamp(
                                    get(value) - increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_RIGHT:
                        if (axis == 0)
                        {
                            set_new_value(
                                value,
                                result,
                                round_and_clamp(
                                    get(value) + increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_UP:
                        if (axis == 1)
                        {
                            set_new_value(
                                value,
                                result,
                                round_and_clamp(
                                    get(value) + increment,
                                    minimum,
                                    maximum,
                                    step));
                            acknowledge_input_event(ctx);
                        }
                        break;
                    case KEY_HOME:
                        set_new_value(value, result, minimum);
                        acknowledge_input_event(ctx);
                        break;
                    case KEY_END:
                        set_new_value(value, result, maximum);
                        acknowledge_input_event(ctx);
                        break;
                }
            }*/
            break;
        }

        case ALIA_CATEGORY_DRAWING: {
            alia_box box = alia_layout_leaf_read(ctx);
            double const current = read_value();
            alia_interaction_status_t const thumb_status
                = alia_element_get_interaction_status(
                    ctx,
                    thumb_id,
                    is_disabled ? ALIA_INTERACTION_STATUS_DISABLED : 0);
            render_slider(
                ctx,
                box,
                axis,
                effective_style,
                minimum,
                maximum,
                current,
                thumb_status);
            break;
        }

        default:
            alia_layout_leaf_read(ctx);
            break;
    }

    (void) data;
    return base_id;
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_element_id
alia_do_slider_d(
    alia_context* ctx,
    double* value,
    double minimum,
    double maximum,
    double step,
    alia_layout_flags_t layout_flags,
    bool vertical,
    alia_slider_style const* style)
{
    return do_slider_impl(
        ctx,
        [value](double v) { *value = v; },
        [value]() { return *value; },
        minimum,
        maximum,
        step,
        layout_flags,
        vertical,
        style);
}

alia_element_id
alia_do_slider_f(
    alia_context* ctx,
    float* value,
    float minimum,
    float maximum,
    float step,
    alia_layout_flags_t layout_flags,
    bool vertical,
    alia_slider_style const* style)
{
    return do_slider_impl(
        ctx,
        [value](double v) { *value = static_cast<float>(v); },
        [value]() { return static_cast<double>(*value); },
        static_cast<double>(minimum),
        static_cast<double>(maximum),
        static_cast<double>(step),
        layout_flags,
        vertical,
        style);
}

alia_slider_style const*
alia_default_slider_style(void)
{
    return &default_slider_style;
}

ALIA_EXTERN_C_END
