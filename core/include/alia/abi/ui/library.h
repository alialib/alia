#ifndef ALIA_ABI_UI_LIBRARY_H
#define ALIA_ABI_UI_LIBRARY_H

#include <alia/abi/context.h>
#include <alia/abi/kernel/animation.h>
#include <alia/abi/kernel/signal.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_switch_style
{
    alia_palette_color off_track;
    alia_palette_color on_track;
    alia_palette_color off_dot;
    alia_palette_color on_dot;
    alia_palette_color off_track_disabled;
    alia_palette_color on_track_disabled;
    alia_palette_color off_dot_disabled;
    alia_palette_color on_dot_disabled;
    alia_palette_color highlight;

    // geometry (logical px unless noted)
    float layout_width;
    float layout_height;
    float track_width;
    float track_height;

    // corner radius as a fraction of track_height (e.g. 0.5 = pill)
    float track_corner_radius_fraction;

    // dot center X from left edge of switch content, logical px
    float dot_center_x_off;
    float dot_center_x_on;

    float dot_radius_off;
    float dot_radius_on;

    float highlight_radius;
    float flare_radius;
} alia_switch_style;

// Fill `out` with the switch style for `seeds`.
// Seeds can be `NULL` to use the default seeds.
void
alia_switch_style_generate(
    alia_switch_style* out, alia_style_seeds const* seeds);

static inline alia_switch_style*
alia_switch_style_default(alia_ui_system* ui)
{
    return (alia_switch_style*) alia_style_default(ui, ALIA_STYLE_SWITCH);
}

static inline alia_switch_style*
alia_switch_style_active(alia_context* ctx)
{
    return (alia_switch_style*) alia_style_active(ctx, ALIA_STYLE_SWITCH);
}

alia_element_id
alia_do_switch(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags);

typedef struct alia_slider_style
{
    alia_palette_color track_color;
    alia_palette_color thumb_color;
    alia_palette_color highlight;

    // total allocated size for the slider leaf, logical px
    float layout_width;
    float layout_height;

    // track thickness (cross-axis) and thumb radius, logical px
    float track_thickness;
    float thumb_radius;
    // extra ring radius when hovered/active, logical px
    float highlight_radius;
} alia_slider_style;

// Fill `out` with the slider style for `seeds`.
// Seeds can be `NULL` to use the default seeds.
void
alia_slider_style_generate(
    alia_slider_style* out, alia_style_seeds const* seeds);

static inline alia_slider_style*
alia_slider_style_default(alia_ui_system* ui)
{
    return (alia_slider_style*) alia_style_default(ui, ALIA_STYLE_SLIDER);
}

static inline alia_slider_style*
alia_slider_style_active(alia_context* ctx)
{
    return (alia_slider_style*) alia_style_active(ctx, ALIA_STYLE_SLIDER);
}

typedef struct alia_radio_style
{
    alia_palette_color outline;
    alia_palette_color dot;
    alia_palette_color outline_disabled;
    alia_palette_color dot_disabled;
    alia_palette_color highlight;

    // size of the radio button, logical px
    float layout_width;
    float layout_height;

    // outer ring radius and inner dot radius, logical px
    float ring_radius;
    float dot_radius;

    // highlight radius when hovered/active, logical px
    float highlight_radius;

    // outer ring outline width, logical px
    float border_width;

    // click flare radius, logical px
    float flare_radius;
} alia_radio_style;

// Fill `out` with the radio style for `seeds`.
// Seeds can be `NULL` to use the default seeds.
void
alia_radio_style_generate(
    alia_radio_style* out, alia_style_seeds const* seeds);

static inline alia_radio_style*
alia_radio_style_default(alia_ui_system* ui)
{
    return (alia_radio_style*) alia_style_default(ui, ALIA_STYLE_RADIO);
}

static inline alia_radio_style*
alia_radio_style_active(alia_context* ctx)
{
    return (alia_radio_style*) alia_style_active(ctx, ALIA_STYLE_RADIO);
}

typedef struct alia_checkbox_state_style
{
    alia_palette_color outline;
    alia_palette_color fill;
} alia_checkbox_state_style;

typedef struct alia_checkbox_style
{
    alia_checkbox_state_style unchecked;
    alia_checkbox_state_style checked;

    alia_checkbox_state_style disabled_unchecked;
    alia_checkbox_state_style disabled_checked;

    alia_palette_color checkmark;
    alia_palette_color disabled_checkmark;

    alia_palette_color highlight;

    // size of the checkbox leaf, logical px
    float layout_width;
    float layout_height;

    // square box size and corner radius, logical px
    float box_size;
    float box_corner_radius;

    // border width for the checkbox square, logical px
    float border_width;

    // icon glyph size for the checkmark, logical px
    float checkmark_size;

    // checkmark glyph in the UI's bound MSDF text engine - When using stock
    // fonts, defaults pick up the font and glyph index from the generated font
    // constants. When using a custom atlas, set these to match the loaded
    // font/glyph. Zero for both skips glyph drawing.
    size_t checkmark_font_index;
    uint32_t checkmark_codepoint;

    // hover/active highlight and click flare radius, logical px
    float highlight_radius;
    float flare_radius;
} alia_checkbox_style;

// Fill `out` with the checkbox style for `seeds`.
// Seeds can be `NULL` to use the default seeds.
void
alia_checkbox_style_generate(
    alia_checkbox_style* out, alia_style_seeds const* seeds);

static inline alia_checkbox_style*
alia_checkbox_style_default(alia_ui_system* ui)
{
    return (alia_checkbox_style*) alia_style_default(ui, ALIA_STYLE_CHECKBOX);
}

static inline alia_checkbox_style*
alia_checkbox_style_active(alia_context* ctx)
{
    return (alia_checkbox_style*) alia_style_active(ctx, ALIA_STYLE_CHECKBOX);
}

typedef struct alia_node_expander_style
{
    alia_palette_color triangle;
    alia_palette_color disabled_triangle;
    alia_palette_color highlight;

    // layout size, logical px - The control does not stretch the glyph beyond
    // this allocated leaf; the glyph is centered in the leaf.
    float layout_width;
    float layout_height;

    // glyph sizing, logical px
    float triangle_side;

    // rotation in degrees (clockwise in screen space), animated from
    // collapsed to expanded
    float collapsed_rotation_degrees;
    float expanded_rotation_degrees;

    // hover/active and click flare parameters, logical px
    float highlight_radius;
    float flare_radius;
} alia_node_expander_style;

// Fill `out` with the node expander style for `seeds`. NULL seeds uses the
// default seeds.
void
alia_node_expander_style_generate(
    alia_node_expander_style* out, alia_style_seeds const* seeds);

static inline alia_node_expander_style*
alia_node_expander_style_default(alia_ui_system* ui)
{
    return (alia_node_expander_style*) alia_style_default(
        ui, ALIA_STYLE_NODE_EXPANDER);
}

static inline alia_node_expander_style*
alia_node_expander_style_active(alia_context* ctx)
{
    return (alia_node_expander_style*) alia_style_active(
        ctx, ALIA_STYLE_NODE_EXPANDER);
}

typedef struct alia_scrollbar_style
{
    alia_palette_color track_color;
    alia_palette_color thumb_color;
    alia_palette_color button_color;
    alia_palette_color glyph_color;
    alia_palette_color highlight;

    float width;
    float button_length;
    float minimum_thumb_length;
    float thumb_corner_radius;
    float line_size;
    // multiplier applied to canonical scroll deltas (default 1)
    float scroll_sensitivity;
} alia_scrollbar_style;

// Fill `out` with the scrollbar style for `seeds`.
// Seeds can be `NULL` to use the default seeds.
void
alia_scrollbar_style_generate(
    alia_scrollbar_style* out, alia_style_seeds const* seeds);

static inline alia_scrollbar_style*
alia_scrollbar_style_default(alia_ui_system* ui)
{
    return (alia_scrollbar_style*) alia_style_default(
        ui, ALIA_STYLE_SCROLLBAR);
}

static inline alia_scrollbar_style*
alia_scrollbar_style_active(alia_context* ctx)
{
    return (alia_scrollbar_style*) alia_style_active(
        ctx, ALIA_STYLE_SCROLLBAR);
}

alia_element_id
alia_do_slider_d(
    alia_context* ctx,
    double* value,
    double minimum,
    double maximum,
    double step,
    alia_layout_flags_t layout_flags,
    bool vertical);

alia_element_id
alia_do_slider_f(
    alia_context* ctx,
    float* value,
    float minimum,
    float maximum,
    float step,
    alia_layout_flags_t layout_flags,
    bool vertical);

alia_element_id
alia_do_radio(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags);

alia_element_id
alia_do_checkbox(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags);

alia_element_id
alia_do_node_expander(
    alia_context* ctx,
    alia_bool_signal* expanded,
    alia_layout_flags_t layout_flags);

void
alia_ui_scroll_view_begin(
    alia_context* ctx,
    alia_layout_flags_t layout_flags,
    uint8_t scrollable_axes,
    uint8_t reserved_axes);
void
alia_ui_scroll_view_end(alia_context* ctx);

// TODO: Decide on a more general return value for this.
bool
alia_ui_collapsible_begin(
    alia_context* ctx,
    alia_bool_signal* expanded,
    alia_layout_flags_t column_flags,
    float offset_factor,
    alia_animated_transition const* transition);
void
alia_ui_collapsible_end(alia_context* ctx);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_LIBRARY_H */
