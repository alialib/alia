#include <alia/ui/color.h>

#include <cmath>
#include <cstdint>
#include <numbers>

static inline float
clamp01(float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

extern "C" {

// sRGB transfer

float
alia_srgb_to_linear(float cs)
{
    cs = clamp01(cs);
    return (cs <= 0.04045f) ? (cs / 12.92f)
                            : std::pow((cs + 0.055f) / 1.055f, 2.4f);
}
extern "C" float
alia_linear_to_srgb(float cl)
{
    cl = clamp01(cl);
    return (cl <= 0.0031308f) ? (12.92f * cl)
                              : (1.055f * std::pow(cl, 1.f / 2.4f) - 0.055f);
}

// authoring helpers

extern "C" alia_srgb8
alia_srgb8_from_hex(const char* s)
{
    if (!s)
        return alia_srgb8{0, 0, 0};
    if (s[0] == '#')
        ++s;
    unsigned v = 0;
    for (int i = 0; i < 6; ++i)
    {
        char c = s[i];
        unsigned d = (c >= '0' && c <= '9') ? (unsigned) (c - '0')
                   : (c >= 'a' && c <= 'f') ? (unsigned) (c - 'a' + 10)
                   : (c >= 'A' && c <= 'F') ? (unsigned) (c - 'A' + 10)
                                            : 0u;
        v = (v << 4) | d;
    }
    return alia_srgb8{
        (uint8_t) ((v >> 16) & 0xFF),
        (uint8_t) ((v >> 8) & 0xFF),
        (uint8_t) ((v) & 0xFF)};
}

// sRGB <-> linear RGB

extern "C" alia_rgb
alia_rgb_from_srgb8(alia_srgb8 c)
{
    return alia_rgb{
        alia_srgb_to_linear(c.r / 255.f),
        alia_srgb_to_linear(c.g / 255.f),
        alia_srgb_to_linear(c.b / 255.f)};
}
extern "C" alia_srgb8
alia_srgb8_from_rgb(alia_rgb c)
{
    float r = alia_linear_to_srgb(clamp01(c.r));
    float g = alia_linear_to_srgb(clamp01(c.g));
    float b = alia_linear_to_srgb(clamp01(c.b));
    return alia_srgb8{
        (uint8_t) std::lround(r * 255.f),
        (uint8_t) std::lround(g * 255.f),
        (uint8_t) std::lround(b * 255.f)};
}

// Linear RGB <-> OKLab

// Björn Ottosson’s reference matrices (linear RGB D65 <-> OKLab)
extern "C" alia_oklab
alia_oklab_from_rgb(alia_rgb c)
{
    const float r = clamp01(c.r), g = clamp01(c.g), b = clamp01(c.b);

    const float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    const float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    const float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

    const float l_ = std::cbrt(l);
    const float m_ = std::cbrt(m);
    const float s_ = std::cbrt(s);

    alia_oklab lab;
    lab.L = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
    lab.a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
    lab.b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;
    return lab;
}
extern "C" alia_rgb
alia_rgb_from_oklab(alia_oklab lab)
{
    const float l_ = lab.L + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
    const float m_ = lab.L - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
    const float s_ = lab.L - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

    const float l = l_ * l_ * l_;
    const float m = m_ * m_ * m_;
    const float s = s_ * s_ * s_;

    alia_rgb out;
    out.r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    out.g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    out.b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
    // (Don’t clamp here; caller decides policy.)
    return out;
}

// OKLab <-> OKLCH

extern "C" alia_oklch
alia_oklch_from_oklab(alia_oklab lab)
{
    alia_oklch lch;
    lch.L = lab.L;
    lch.C = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    lch.h = std::atan2(lab.b, lab.a);
    if (lch.h < 0.f)
        lch.h += 2.f * std::numbers::pi_v<float>;
    return lch;
}
extern "C" alia_oklab
alia_oklab_from_oklch(alia_oklch lch)
{
    alia_oklab lab;
    lab.L = lch.L;
    lab.a = lch.C * std::cos(lch.h);
    lab.b = lch.C * std::sin(lch.h);
    return lab;
}

// convenience: sRGB8 -> OKLCH

extern "C" alia_oklch
alia_oklch_from_srgb8(alia_srgb8 c)
{
    return alia_oklch_from_oklab(alia_oklab_from_rgb(alia_rgb_from_srgb8(c)));
}

// alpha / compositing (linear premultiplied)

// premultiply
extern "C" alia_rgba
alia_rgba_from_rgb_alpha(alia_rgb c, float a)
{
    a = clamp01(a);
    return alia_rgba{c.r * a, c.g * a, c.b * a, a};
}

// modulate (rgb*a, a*a)
extern "C" alia_rgba
alia_apply_alpha_rgba(alia_rgba c, float a)
{
    a = clamp01(a);
    return alia_rgba{c.r * a, c.g * a, c.b * a, c.a * a};
}

// lerp

extern "C" alia_rgb
alia_lerp_rgb(alia_rgb a, alia_rgb b, float t)
{
    return alia_rgb{
        a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}
extern "C" alia_rgba
alia_lerp_rgba(alia_rgba a, alia_rgba b, float t)
{
    return alia_rgba{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t};
}

// relative luminance (W3C)

extern "C" float
alia_relative_luminance_rgb(alia_rgb c)
{
    // Rec. 709 coefficients in *linear* light
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}
extern "C" float
alia_relative_luminance_rgba(alia_rgba c)
{
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

} // extern "C"
