#ifndef ALIA_ABI_BASE_COLOR_H
#define ALIA_ABI_BASE_COLOR_H

#include <alia/abi/prelude.h>

#define ALIA_COLOR_API_VERSION 0

ALIA_EXTERN_C_BEGIN

// sRGB
typedef struct alia_srgb8
{
    uint8_t r, g, b;
} alia_srgb8;

// sRGBA
typedef struct alia_srgba8
{
    uint8_t r, g, b, a;
} alia_srgba8;

// linear RGB
typedef struct alia_rgb
{
    float r, g, b;
} alia_rgb;

// linear RGBA, premultiplied alpha
typedef struct alia_rgba
{
    float r, g, b, a;
} alia_rgba;

// OKLab
typedef struct alia_oklab
{
    float l, a, b;
} alia_oklab;

// OKLCH
typedef struct alia_oklch
{
    float l, c, h;
} alia_oklch;

// sRGB transfer (single component in [0,1])
float
alia_linear_component_from_srgb(float cs);
float
alia_srgb_component_from_linear(float cl);

// sRGB <-> linear RGB
alia_rgb
alia_rgb_from_srgb8(alia_srgb8 c);
alia_srgb8
alia_srgb8_from_rgb(alia_rgb c);

// sRGBA <-> linear RGBA
alia_rgba
alia_rgba_from_srgba8(alia_srgba8 c);
alia_srgba8
alia_srgba8_from_rgba(alia_rgba c);

// linear RGB <-> OKLab
alia_oklab
alia_oklab_from_rgb(alia_rgb c);
alia_rgb
alia_rgb_from_oklab(alia_oklab lab);

// OKLab <-> OKLCH
alia_oklch
alia_oklch_from_oklab(alia_oklab lab);
alia_oklab
alia_oklab_from_oklch(alia_oklch lch);

// perceptual chroma clipper
alia_oklab
alia_oklab_clip_chroma_to_srgb(alia_oklab color);

// linear RGB <-> OKLCH
alia_oklch
alia_oklch_from_rgb(alia_rgb c);

// convenience

static inline alia_oklch
alia_oklch_from_rgb(alia_rgb c)
{
    return alia_oklch_from_oklab(alia_oklab_from_rgb(c));
}

static inline alia_oklch
alia_oklch_from_srgb8(alia_srgb8 c)
{
    return alia_oklch_from_rgb(alia_rgb_from_srgb8(c));
}

static inline alia_srgb8
alia_srgb8_from_oklab(alia_oklab lab)
{
    return alia_srgb8_from_rgb(alia_rgb_from_oklab(lab));
}

alia_srgb8
alia_srgb8_from_hex(const char* hex6_or_hash_hex6);

// alpha / compositing helpers (linear premultiplied)

alia_rgba
alia_rgba_from_rgb_alpha(alia_rgb c, float a);

// modulate (rgb*a, a*a)
alia_rgba
alia_apply_alpha_rgba(alia_rgba c, float a);

alia_oklch
alia_lerp_oklch(alia_oklch a, alia_oklch b, float t);

// raw lerp - Just does the raw math in linear RGB space.
alia_rgb
alia_lerp_rgb_raw(alia_rgb a, alia_rgb b, float t);
alia_rgba
alia_lerp_rgba_raw(alia_rgba a, alia_rgba b, float t);

// lerp via OKLCH
alia_srgb8
alia_lerp_srgb8_via_oklch(alia_srgb8 a, alia_srgb8 b, float t);

// relative luminance
float
alia_relative_luminance_rgb(alia_rgb c);
float
alia_relative_luminance_rgba(alia_rgba c);

inline float
alia_relative_luminance_srgb8(alia_srgb8 c)
{
    return alia_relative_luminance_rgb(alia_rgb_from_srgb8(c));
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_BASE_COLOR_H */
