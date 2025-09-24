#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALIA_COLOR_API_VERSION 0

// sRGB
typedef struct
{
    uint8_t r, g, b;
} alia_srgb8;

// linear RGB
typedef struct
{
    float r, g, b;
} alia_rgb;

// linear RGBA, premultiplied alpha
typedef struct
{
    float r, g, b, a;
} alia_rgba;

// OKLab
typedef struct
{
    float l, a, b;
} alia_oklab;

// OKLCH
typedef struct
{
    float l, c, h;
} alia_oklch;

// sRGB transfer (single component in [0,1])
float
alia_srgb_to_linear(float cs);
float
alia_linear_to_srgb(float cl);

// sRGB <-> linear RGB
alia_rgb
alia_rgb_from_srgb8(alia_srgb8 c);
alia_srgb8
alia_srgb8_from_rgb(alia_rgb c);

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

// linear RGB <-> OKLCH
alia_oklch
alia_oklch_from_rgb(alia_rgb c);
alia_rgb
alia_rgb_from_oklch(alia_oklch lch);

// convenience

alia_oklch
alia_oklch_from_srgb8(alia_srgb8 c);

alia_srgb8
alia_srgb8_from_hex(const char* hex6_or_hash_hex6);

// alpha / compositing helpers (linear premultiplied)

// premultiply
alia_rgba
alia_rgba_from_rgb_alpha(alia_rgb c, float a); // premultiply

// modulate (rgb*a, a*a)
alia_rgba
alia_apply_alpha_rgba(alia_rgba c, float a); // modulate (rgb*a, a*a)

// lerp
alia_rgb
alia_lerp_rgb(alia_rgb a, alia_rgb b, float t);
alia_rgba
alia_lerp_rgba(alia_rgba a, alia_rgba b, float t);

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

#ifdef __cplusplus
}
#endif
