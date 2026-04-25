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

static inline alia_srgb8
alia_srgb8_make(uint8_t r, uint8_t g, uint8_t b)
{
    return ALIA_BRACED_INIT(alia_srgb8, r, g, b);
}

// sRGBA
typedef struct alia_srgba8
{
    uint8_t r, g, b, a;
} alia_srgba8;

static inline alia_srgba8
alia_srgba8_make(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ALIA_BRACED_INIT(alia_srgba8, r, g, b, a);
}

// linear RGB
typedef struct alia_rgb
{
    float r, g, b;
} alia_rgb;

static inline alia_rgb
alia_rgb_make(float r, float g, float b)
{
    return ALIA_BRACED_INIT(alia_rgb, r, g, b);
}

// linear RGBA, premultiplied alpha
typedef struct alia_rgba
{
    float r, g, b, a;
} alia_rgba;

static inline alia_rgba
alia_rgba_make(float r, float g, float b, float a)
{
    return ALIA_BRACED_INIT(alia_rgba, r, g, b, a);
}

// OKLab
typedef struct alia_oklab
{
    float l, a, b;
} alia_oklab;

static inline alia_oklab
alia_oklab_make(float l, float a, float b)
{
    return ALIA_BRACED_INIT(alia_oklab, l, a, b);
}

// OKLCH
typedef struct alia_oklch
{
    float l, c, h;
} alia_oklch;

static inline alia_oklch
alia_oklch_make(float l, float c, float h)
{
    return ALIA_BRACED_INIT(alia_oklch, l, c, h);
}

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

// Clamp a calculated OKLCH value to the sRGB gamut, returning an sRGB8.
alia_srgb8
alia_srgb8_from_unclamped_oklch(alia_oklch lch);

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

static inline alia_rgba
alia_rgba_from_rgb(alia_rgb c)
{
    return alia_rgba_make(c.r, c.g, c.b, 1.0f);
}

static inline alia_srgba8
alia_srgba8_from_srgb8_alpha(alia_srgb8 c, uint8_t a)
{
    return alia_srgba8_make(c.r, c.g, c.b, a);
}

static inline alia_srgba8
alia_srgba8_from_srgb8(alia_srgb8 c)
{
    return alia_srgba8_make(c.r, c.g, c.b, 0xff);
}

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

static inline float
alia_relative_luminance_srgb8(alia_srgb8 c)
{
    return alia_relative_luminance_rgb(alia_rgb_from_srgb8(c));
}

static inline float
alia_relative_luminance_ratio(float luminance_a, float luminance_b)
{
    if (luminance_a < luminance_b)
        return (luminance_b + 0.05f) / (luminance_a + 0.05f);
    else
        return (luminance_a + 0.05f) / (luminance_b + 0.05f);
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_BASE_COLOR_H */
