#include <alia/abi/base/color.h>

#include <cmath>
#include <cstdint>
#include <numbers>

ALIA_EXTERN_C_BEGIN

// TODO: Move these somewhere in base/.
static inline float
clamp01(float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

// sRGB transfer

float
alia_linear_component_from_srgb(float cs)
{
    cs = clamp01(cs);
    return (cs <= 0.04045f) ? (cs / 12.92f)
                            : std::pow((cs + 0.055f) / 1.055f, 2.4f);
}
float
alia_srgb_component_from_linear(float cl)
{
    cl = clamp01(cl);
    return (cl <= 0.0031308f) ? (12.92f * cl)
                              : (1.055f * std::pow(cl, 1.f / 2.4f) - 0.055f);
}

const float alia_srgb_to_linear_lut[256] = {
    0.000000000f, 0.000303527f, 0.000607054f, 0.000910581f, 0.001214108f,
    0.001517635f, 0.001821162f, 0.002124689f, 0.002428216f, 0.002731743f,
    0.003035270f, 0.003346536f, 0.003676507f, 0.004024717f, 0.004391442f,
    0.004776953f, 0.005181517f, 0.005605392f, 0.006048833f, 0.006512091f,
    0.006995410f, 0.007499032f, 0.008023193f, 0.008568126f, 0.009134059f,
    0.009721218f, 0.010329823f, 0.010960094f, 0.011612245f, 0.012286488f,
    0.012983033f, 0.013702083f, 0.014443844f, 0.015208514f, 0.015996294f,
    0.016807375f, 0.017641954f, 0.018500220f, 0.019382361f, 0.020288562f,
    0.021219010f, 0.022173885f, 0.023153367f, 0.024157632f, 0.025186859f,
    0.026241222f, 0.027320892f, 0.028426040f, 0.029556835f, 0.030713445f,
    0.031896032f, 0.033104766f, 0.034339808f, 0.035601314f, 0.036889449f,
    0.038204372f, 0.039546236f, 0.040915199f, 0.042311411f, 0.043735031f,
    0.045186203f, 0.046665087f, 0.048171826f, 0.049706567f, 0.051269457f,
    0.052860647f, 0.054480277f, 0.056128491f, 0.057805430f, 0.059511237f,
    0.061246052f, 0.063010015f, 0.064803265f, 0.066625938f, 0.068478167f,
    0.070360094f, 0.072271854f, 0.074213572f, 0.076185383f, 0.078187421f,
    0.080219820f, 0.082282707f, 0.084376208f, 0.086500458f, 0.088655584f,
    0.090841711f, 0.093058966f, 0.095307469f, 0.097587347f, 0.099898726f,
    0.102241732f, 0.104616486f, 0.107023105f, 0.109461710f, 0.111932427f,
    0.114435375f, 0.116970666f, 0.119538426f, 0.122138776f, 0.124771819f,
    0.127437681f, 0.130136475f, 0.132868320f, 0.135633335f, 0.138431609f,
    0.141263291f, 0.144128472f, 0.147027269f, 0.149959788f, 0.152926147f,
    0.155926466f, 0.158960834f, 0.162029371f, 0.165132195f, 0.168269396f,
    0.171441108f, 0.174647406f, 0.177888423f, 0.181164250f, 0.184474990f,
    0.187820777f, 0.191201687f, 0.194617838f, 0.198069319f, 0.201556250f,
    0.205078736f, 0.208636865f, 0.212230757f, 0.215860501f, 0.219526201f,
    0.223227963f, 0.226965874f, 0.230740055f, 0.234550580f, 0.238397568f,
    0.242281124f, 0.246201321f, 0.250158280f, 0.254152089f, 0.258182853f,
    0.262250662f, 0.266355604f, 0.270497799f, 0.274677306f, 0.278894275f,
    0.283148736f, 0.287440836f, 0.291770637f, 0.296138257f, 0.300543785f,
    0.304987311f, 0.309468925f, 0.313988715f, 0.318546772f, 0.323143214f,
    0.327778101f, 0.332451522f, 0.337163627f, 0.341914415f, 0.346704066f,
    0.351532608f, 0.356400132f, 0.361306787f, 0.366252601f, 0.371237695f,
    0.376262128f, 0.381326020f, 0.386429429f, 0.391572475f, 0.396755219f,
    0.401977777f, 0.407240212f, 0.412542611f, 0.417885065f, 0.423267663f,
    0.428690493f, 0.434153646f, 0.439657182f, 0.445201188f, 0.450785786f,
    0.456411034f, 0.462076992f, 0.467783809f, 0.473531485f, 0.479320168f,
    0.485149950f, 0.491020858f, 0.496932983f, 0.502886474f, 0.508881330f,
    0.514917672f, 0.520995557f, 0.527115107f, 0.533276379f, 0.539479494f,
    0.545724452f, 0.552011430f, 0.558340371f, 0.564711511f, 0.571124852f,
    0.577580452f, 0.584078431f, 0.590618849f, 0.597201765f, 0.603827357f,
    0.610495567f, 0.617206573f, 0.623960376f, 0.630757153f, 0.637596846f,
    0.644479692f, 0.651405632f, 0.658374846f, 0.665387273f, 0.672443151f,
    0.679542482f, 0.686685324f, 0.693871737f, 0.701101899f, 0.708375752f,
    0.715693474f, 0.723055124f, 0.730460763f, 0.737910390f, 0.745404184f,
    0.752942204f, 0.760524511f, 0.768151164f, 0.775822222f, 0.783537805f,
    0.791297913f, 0.799102724f, 0.806952238f, 0.814846575f, 0.822785735f,
    0.830769897f, 0.838799000f, 0.846873224f, 0.854992628f, 0.863157213f,
    0.871367097f, 0.879622400f, 0.887923121f, 0.896269381f, 0.904661179f,
    0.913098633f, 0.921581864f, 0.930110872f, 0.938685715f, 0.947306514f,
    0.955973327f, 0.964686275f, 0.973445296f, 0.982250571f, 0.991102099f,
    1.000000000f,
};

// authoring helpers

alia_srgb8
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

alia_rgb
alia_rgb_from_srgb8(alia_srgb8 c)
{
    return alia_rgb{
        alia_srgb_to_linear_lut[c.r],
        alia_srgb_to_linear_lut[c.g],
        alia_srgb_to_linear_lut[c.b]};
}
alia_srgb8
alia_srgb8_from_rgb(alia_rgb c)
{
    float r = alia_srgb_component_from_linear(clamp01(c.r));
    float g = alia_srgb_component_from_linear(clamp01(c.g));
    float b = alia_srgb_component_from_linear(clamp01(c.b));
    return alia_srgb8{
        (uint8_t) std::lround(r * 255.f),
        (uint8_t) std::lround(g * 255.f),
        (uint8_t) std::lround(b * 255.f)};
}

// sRGBA <-> linear RGBA

alia_rgba
alia_rgba_from_srgba8(alia_srgba8 c)
{
    return alia_rgba{
        alia_srgb_to_linear_lut[c.r],
        alia_srgb_to_linear_lut[c.g],
        alia_srgb_to_linear_lut[c.b],
        c.a * (1.f / 255.f)};
}
alia_srgba8
alia_srgba8_from_rgba(alia_rgba c)
{
    float r = alia_srgb_component_from_linear(clamp01(c.r));
    float g = alia_srgb_component_from_linear(clamp01(c.g));
    float b = alia_srgb_component_from_linear(clamp01(c.b));
    return alia_srgba8{
        (uint8_t) std::lround(r * 255.f),
        (uint8_t) std::lround(g * 255.f),
        (uint8_t) std::lround(b * 255.f),
        (uint8_t) std::lround(clamp01(c.a) * 255.f)};
}

// Linear RGB <-> OKLab

// Björn Ottosson’s reference matrices (linear RGB D65 <-> OKLab)
alia_oklab
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
    lab.l = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
    lab.a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
    lab.b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;
    return lab;
}
alia_rgb
alia_rgb_from_oklab(alia_oklab lab)
{
    const float l_ = lab.l + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
    const float m_ = lab.l - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
    const float s_ = lab.l - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

    const float l = l_ * l_ * l_;
    const float m = m_ * m_ * m_;
    const float s = s_ * s_ * s_;

    alia_rgb out;
    out.r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    out.g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    out.b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
    // (Don’t clamp here; caller decides policy.)
    return out;
}

// OKLab <-> OKLCH

alia_oklch
alia_oklch_from_oklab(alia_oklab lab)
{
    alia_oklch lch;
    lch.l = lab.l;
    lch.c = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    lch.h = std::atan2(lab.b, lab.a);
    if (lch.h < 0.f)
        lch.h += 2.f * std::numbers::pi_v<float>;
    return lch;
}
alia_oklab
alia_oklab_from_oklch(alia_oklch lch)
{
    alia_oklab lab;
    lab.l = lch.l;
    lab.a = lch.c * std::cos(lch.h);
    lab.b = lch.c * std::sin(lch.h);
    return lab;
}

// Evaluate the Oklab to RGB polynomial for a single channel.
// Return the distance from the target boundary (0.0 or 1.0).
static inline float
alia_oklab_channel_err(
    float l,
    float a,
    float b,
    float c_scale,
    float m1,
    float m2,
    float m3,
    float target)
{
    float a_scaled = a * c_scale;
    float b_scaled = b * c_scale;

    // inverse M2 (Oklab to LMS)
    float l_ = l + 0.3963377774f * a_scaled + 0.2158037573f * b_scaled;
    float m_ = l - 0.1055613458f * a_scaled - 0.0638541728f * b_scaled;
    float s_ = l - 0.0894841775f * a_scaled - 1.2914855480f * b_scaled;

    // Cube (Undo perceptual curve)
    float l_cu = l_ * l_ * l_;
    float m_cu = m_ * m_ * m_;
    float s_cu = s_ * s_ * s_;

    // inverse M1 (LMS to linear RGB channel)
    float channel_val = m1 * l_cu + m2 * m_cu + m3 * s_cu;

    return channel_val - target;
}

alia_oklab
alia_oklab_clip_chroma_to_srgb(alia_oklab color)
{
    // Fast Path: Check if we are already inside the sRGB gamut
    alia_rgb check = alia_rgb_from_oklab(color);
    if (check.r >= 0.0f && check.r <= 1.0f && check.g >= 0.0f
        && check.g <= 1.0f && check.b >= 0.0f && check.b <= 1.0f)
    {
        return color;
    }

    // The color is out of bounds. We must find the maximum safe chroma
    // multiplier (t). We start at t = 1.0 (current chroma) and reduce it until
    // all channels are safe.
    float t = 1.0f;

    // Check R, G, and B independently and keep the smallest safe 't'...

    // red channel (M1 row 1)
    if (check.r < 0.0f || check.r > 1.0f)
    {
        // the target boundary we are trying to hit
        float target = (check.r < 0.0f) ? 0.0f : 1.0f;
        // simple binary search / secant fallback for the exact root -
        // Halley's method requires the 1st and 2nd derivatives of the cubic
        // function, which expands to 30+ lines of math per channel. A tight
        // bounded search is often mechanically faster in instruction cache.
        float t_low = 0.0f;
        float t_high = t;
        for (int i = 0; i < 8; ++i)
        { // 8 iterations guarantees visual perfection
            float t_mid = (t_low + t_high) * 0.5f;
            float err = alia_oklab_channel_err(
                color.l,
                color.a,
                color.b,
                t_mid,
                4.0767416621f,
                -3.3077115913f,
                0.2309699292f,
                target);
            if ((err > 0.0f && target == 1.0f)
                || (err < 0.0f && target == 0.0f))
            {
                t_high = t_mid;
            }
            else
            {
                t_low = t_mid;
            }
        }
        t = t_low;
    }

    // green channel (M1 row 2)
    if (check.g < 0.0f || check.g > 1.0f)
    {
        float target = (check.g < 0.0f) ? 0.0f : 1.0f;
        float t_low = 0.0f;
        float t_high = t;
        for (int i = 0; i < 8; ++i)
        {
            float t_mid = (t_low + t_high) * 0.5f;
            float err = alia_oklab_channel_err(
                color.l,
                color.a,
                color.b,
                t_mid,
                -1.2684380046f,
                2.6097574011f,
                -0.3413193965f,
                target);
            if ((err > 0.0f && target == 1.0f)
                || (err < 0.0f && target == 0.0f))
                t_high = t_mid;
            else
                t_low = t_mid;
        }
        t = t_low;
    }

    // blue channel (M1 row 3)
    if (check.b < 0.0f || check.b > 1.0f)
    {
        float target = (check.b < 0.0f) ? 0.0f : 1.0f;
        float t_low = 0.0f;
        float t_high = t;
        for (int i = 0; i < 8; ++i)
        {
            float t_mid = (t_low + t_high) * 0.5f;
            float err = alia_oklab_channel_err(
                color.l,
                color.a,
                color.b,
                t_mid,
                -0.0041960863f,
                -0.7034186147f,
                1.7076147010f,
                target);
            if ((err > 0.0f && target == 1.0f)
                || (err < 0.0f && target == 0.0f))
                t_high = t_mid;
            else
                t_low = t_mid;
        }
        t = t_low;
    }

    // Apply the maximum safe chroma scalar to the `a` and `b` channels.
    alia_oklab result = color;
    result.a *= t;
    result.b *= t;

    return result;
}

// alpha / compositing (linear premultiplied)
// TODO: Inline this.
alia_rgba
alia_rgba_from_rgb_alpha(alia_rgb c, float a)
{
    a = clamp01(a); // TODO: Is this necessary?
    return alia_rgba{c.r * a, c.g * a, c.b * a, a};
}

// modulate (rgb*a, a*a)
// TODO: Inline this.
alia_rgba
alia_apply_alpha_rgba(alia_rgba c, float a)
{
    a = clamp01(a); // TODO: Is this necessary?
    return alia_rgba{c.r * a, c.g * a, c.b * a, c.a * a};
}

// lerp

alia_rgb
alia_lerp_rgb_raw(alia_rgb a, alia_rgb b, float t)
{
    return alia_rgb{
        a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}
alia_rgba
alia_lerp_rgba_raw(alia_rgba a, alia_rgba b, float t)
{
    return alia_rgba{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t};
}

alia_srgb8
alia_lerp_srgb8_via_oklch(alia_srgb8 a, alia_srgb8 b, float t)
{
    alia_oklch a_oklch = alia_oklch_from_srgb8(a);
    alia_oklch b_oklch = alia_oklch_from_srgb8(b);
    alia_oklch lerped_oklch = alia_lerp_oklch(a_oklch, b_oklch, t);
    alia_oklab lerped_oklab = alia_oklab_from_oklch(lerped_oklch);
    alia_oklab clipped_oklab = alia_oklab_clip_chroma_to_srgb(lerped_oklab);
    return alia_srgb8_from_oklab(clipped_oklab);
}

alia_oklch
alia_lerp_oklch(alia_oklch a, alia_oklch b, float t)
{
    alia_oklch res;

    const float pi = 3.14159265359f;

    // Lightness and chroma lerp linearly.
    res.l = a.l + (b.l - a.l) * t;
    res.c = a.c + (b.c - a.c) * t;

    // Hue requires shortest-path interpolation on a circle.
    float h0 = a.h;
    float h1 = b.h;
    float delta = h1 - h0;

    // If the distance is more than half a circle, wrap around.
    if (delta > pi)
    {
        h0 += 2.0f * pi;
    }
    else if (delta < -pi)
    {
        h1 += 2.0f * pi;
    }

    res.h = h0 + (h1 - h0) * t;
    // Normalize the result within [-pi, pi].
    if (res.h > pi)
        res.h -= 2.0f * pi;
    else if (res.h < -pi)
        res.h += 2.0f * pi;

    return res;
}

// relative luminance (W3C)

float
alia_relative_luminance_rgb(alia_rgb c)
{
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}
float
alia_relative_luminance_rgba(alia_rgba c)
{
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

ALIA_EXTERN_C_END
