#ifndef ALIA_ABI_UI_LIBRARY_H
#define ALIA_ABI_UI_LIBRARY_H

#include <alia/abi/context.h>
#include <alia/abi/kernel/signal.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/palette.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_switch_style
{
    uint8_t off_track;
    uint8_t on_track;
    uint8_t off_dot;
    uint8_t on_dot;
    uint8_t highlight;

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
    uint8_t track_color;
    uint8_t thumb_color;
    uint8_t highlight;

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
    uint8_t outline;
    uint8_t dot;
    uint8_t highlight;

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

alia_switch_style const*
alia_default_switch_style(void);
alia_slider_style const*
alia_default_slider_style(void);
alia_radio_style const*
alia_default_radio_style(void);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_LIBRARY_H */
