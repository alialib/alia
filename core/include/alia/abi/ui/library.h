#ifndef ALIA_ABI_UI_LIBRARY_H
#define ALIA_ABI_UI_LIBRARY_H

#include <alia/abi/context.h>
#include <alia/abi/kernel/animation.h>
#include <alia/abi/kernel/signal.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/palette.h>

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

    // Geometry (logical px unless noted). Switch is drawn at fixed size inside
    // allocated placement (centered); it does not stretch with allocated size.
    float layout_width;
    float layout_height;
    float track_width;
    float track_height;

    // Corner radius = fraction * track_height (e.g. 0.5 = pill).
    float track_corner_radius_fraction;

    // dot center X from left edge of switch content (logical px)
    float dot_center_x_off;
    float dot_center_x_on;

    float dot_radius_off;
    float dot_radius_on;

    float highlight_radius;
    float flare_radius;
} alia_switch_style;

alia_element_id
alia_do_switch(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags,
    alia_switch_style const* style);

typedef struct alia_slider_style
{
    alia_palette_color track_color;
    alia_palette_color thumb_color;
    alia_palette_color highlight;

    // Total allocated size for the slider leaf (logical px).
    float layout_width;
    float layout_height;

    // Track thickness (cross-axis) and thumb radius (logical px).
    float track_thickness;
    float thumb_radius;
    // Extra ring radius when hovered/active (logical px).
    float highlight_radius;
} alia_slider_style;

typedef struct alia_radio_style
{
    alia_palette_color outline;
    alia_palette_color dot;
    alia_palette_color outline_disabled;
    alia_palette_color dot_disabled;
    alia_palette_color highlight;

    // size of the radio button (logical px)
    float layout_width;
    float layout_height;

    // outer ring radius and inner dot radius (logical px)
    float ring_radius;
    float dot_radius;

    // radius of the highlight when hovered/active (logical px)
    float highlight_radius;

    // width of the outer ring outline (logical px)
    float border_width;

    // radius used for click flare effects (logical px)
    float flare_radius;
} alia_radio_style;

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

    // size of the checkbox leaf (logical px)
    float layout_width;
    float layout_height;

    // square box size + corner radius (logical px)
    float box_size;
    float box_corner_radius;

    // border width for the checkbox square (logical px)
    float border_width;

    // icon glyph size for checkmark (logical px)
    float checkmark_size;

    // radius used for hover/active highlight + click flare (logical px)
    float highlight_radius;
    float flare_radius;
} alia_checkbox_style;

typedef struct alia_node_expander_style
{
    alia_palette_color triangle;
    alia_palette_color disabled_triangle;
    alia_palette_color highlight;

    // Layout (logical px). The control does not stretch the glyph beyond this
    // allocated leaf (glyph is centered in the leaf).
    float layout_width;
    float layout_height;

    // Glyph sizing (logical px).
    float triangle_side;

    // Degrees (clockwise in screen space), animated from collapsed to
    // expanded.
    float collapsed_rotation_degrees;
    float expanded_rotation_degrees;

    // Hover/active + click flare parameters (logical px).
    float highlight_radius;
    float flare_radius;
} alia_node_expander_style;

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
    float scroll_input_scale;
} alia_scrollbar_style;

alia_element_id
alia_do_slider_d(
    alia_context* ctx,
    double* value,
    double minimum,
    double maximum,
    double step,
    alia_layout_flags_t layout_flags,
    bool vertical,
    alia_slider_style const* style);

alia_element_id
alia_do_slider_f(
    alia_context* ctx,
    float* value,
    float minimum,
    float maximum,
    float step,
    alia_layout_flags_t layout_flags,
    bool vertical,
    alia_slider_style const* style);

alia_element_id
alia_do_radio(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags,
    alia_radio_style const* style);

alia_element_id
alia_do_checkbox(
    alia_context* ctx,
    alia_bool_signal* value,
    alia_layout_flags_t layout_flags,
    alia_checkbox_style const* style);

alia_element_id
alia_do_node_expander(
    alia_context* ctx,
    alia_bool_signal* expanded,
    alia_layout_flags_t layout_flags,
    alia_node_expander_style const* style);

void
alia_ui_scroll_view_begin(
    alia_context* ctx,
    alia_layout_flags_t layout_flags,
    uint8_t scrollable_axes,
    uint8_t reserved_axes,
    alia_scrollbar_style const* style);
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

alia_switch_style const*
alia_default_switch_style(void);
alia_slider_style const*
alia_default_slider_style(void);
alia_radio_style const*
alia_default_radio_style(void);
alia_checkbox_style const*
alia_default_checkbox_style(void);
alia_node_expander_style const*
alia_default_node_expander_style(void);
alia_scrollbar_style const*
alia_default_scrollbar_style(void);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_LIBRARY_H */
