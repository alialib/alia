#ifndef ALIA_ABI_UI_LIBRARY_H
#define ALIA_ABI_UI_LIBRARY_H

#include <alia/abi/context.h>
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
    bool* state, // TODO: Use `alia_signal_bool` instead.
    alia_layout_flags_t layout_flags,
    alia_switch_style const* style);

// Slider: palette indices + geometry (logical px). Horizontal when vertical is false.
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

// vertical: true = value maps to Y axis (bottom = min), false = X axis (left = min).
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

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_LIBRARY_H */
