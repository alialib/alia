#include <alia/abi/base/color.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <math.h>
#include <stddef.h>

static void
check_near_f(float got, float expected, float eps)
{
    TEST_CHECK(fabsf(got - expected) <= eps);
}

static void
check_near_rgb(alia_rgb got, alia_rgb expected, float eps)
{
    check_near_f(got.r, expected.r, eps);
    check_near_f(got.g, expected.g, eps);
    check_near_f(got.b, expected.b, eps);
}

static void
check_near_rgba(alia_rgba got, alia_rgba expected, float eps)
{
    check_near_f(got.r, expected.r, eps);
    check_near_f(got.g, expected.g, eps);
    check_near_f(got.b, expected.b, eps);
    check_near_f(got.a, expected.a, eps);
}

static void
check_srgb8_eq(alia_srgb8 got, alia_srgb8 expected)
{
    TEST_CHECK(got.r == expected.r);
    TEST_CHECK(got.g == expected.g);
    TEST_CHECK(got.b == expected.b);
}

static void
check_srgba8_eq(alia_srgba8 got, alia_srgba8 expected)
{
    TEST_CHECK(got.r == expected.r);
    TEST_CHECK(got.g == expected.g);
    TEST_CHECK(got.b == expected.b);
    TEST_CHECK(got.a == expected.a);
}

void
color_tests(void)
{
    float const eps = 0.0001f;
    float const eps_rt = 0.02f; /* sRGB8 <-> OKLab round-trip */

    /* struct makers */
    {
        alia_srgb8 s = alia_srgb8_make(10, 20, 30);
        check_srgb8_eq(s, (alia_srgb8){10, 20, 30});
        alia_srgba8 sa = alia_srgba8_make(1, 2, 3, 4);
        check_srgba8_eq(sa, (alia_srgba8){1, 2, 3, 4});
        alia_rgb rgb = alia_rgb_make(0.25f, 0.5f, 0.75f);
        check_near_rgb(rgb, (alia_rgb){0.25f, 0.5f, 0.75f}, 0.f);
        alia_rgba rgba = alia_rgba_make(0.1f, 0.2f, 0.3f, 0.4f);
        check_near_rgba(rgba, (alia_rgba){0.1f, 0.2f, 0.3f, 0.4f}, 0.f);
        alia_oklab lab = alia_oklab_make(0.5f, 0.05f, -0.05f);
        check_near_f(lab.l, 0.5f, 0.f);
        check_near_f(lab.a, 0.05f, 0.f);
        check_near_f(lab.b, -0.05f, 0.f);
        alia_oklch lch = alia_oklch_make(0.6f, 0.1f, 1.0f);
        check_near_f(lch.l, 0.6f, 0.f);
        check_near_f(lch.c, 0.1f, 0.f);
        check_near_f(lch.h, 1.0f, 0.f);
    }

    /* sRGB transfer endpoints */
    check_near_f(alia_linear_component_from_srgb(0.f), 0.f, eps);
    check_near_f(alia_linear_component_from_srgb(1.f), 1.f, eps);
    check_near_f(alia_srgb_component_from_linear(0.f), 0.f, eps);
    check_near_f(alia_srgb_component_from_linear(1.f), 1.f, eps);

    /* sRGB8 <-> linear RGB round-trip (uses LUT forward, piecewise reverse) */
    {
        alia_srgb8 const samples[] = {
            {0x00, 0x00, 0x00},
            {0xff, 0xff, 0xff},
            {0x12, 0x34, 0x56},
            {0x80, 0x40, 0xc0},
        };
        for (size_t i = 0; i < sizeof samples / sizeof samples[0]; ++i)
        {
            alia_srgb8 orig = samples[i];
            alia_rgb lin = alia_rgb_from_srgb8(orig);
            alia_srgb8 back = alia_srgb8_from_rgb(lin);
            check_srgb8_eq(back, orig);
        }
    }

    /* sRGBA8 <-> linear RGBA */
    {
        alia_srgba8 c = alia_srgba8_make(0x33, 0x66, 0x99, 0xcc);
        alia_rgba lin = alia_rgba_from_srgba8(c);
        alia_srgba8 back = alia_srgba8_from_rgba(lin);
        check_srgba8_eq(back, c);
    }

    /* inline sRGBA helpers */
    {
        alia_srgb8 s = alia_srgb8_make(0xab, 0xcd, 0xef);
        check_srgba8_eq(
            alia_srgba8_from_srgb8_alpha(s, 0x42),
            alia_srgba8_make(0xab, 0xcd, 0xef, 0x42));
        check_srgba8_eq(
            alia_srgba8_from_srgb8(s), alia_srgba8_make(0xab, 0xcd, 0xef, 0xff));
        alia_rgb red = alia_rgb_make(1.f, 0.f, 0.f);
        check_near_rgba(
            alia_rgba_from_rgb(red),
            alia_rgba_make(1.f, 0.f, 0.f, 1.f),
            0.f);
    }

    /* hex */
    check_srgb8_eq(alia_srgb8_from_hex(NULL), alia_srgb8_make(0, 0, 0));
    check_srgb8_eq(alia_srgb8_from_hex("ff0000"), alia_srgb8_make(0xff, 0, 0));
    check_srgb8_eq(alia_srgb8_from_hex("#00ff00"), alia_srgb8_make(0, 0xff, 0));
    check_srgb8_eq(alia_srgb8_from_hex("0000FF"), alia_srgb8_make(0, 0, 0xff));

    /* premultiplied alpha helpers */
    {
        alia_rgb c = alia_rgb_make(0.25f, 0.5f, 0.75f);
        check_near_rgba(
            alia_rgba_from_rgb_alpha(c, 0.5f),
            alia_rgba_make(0.125f, 0.25f, 0.375f, 0.5f),
            eps);
        alia_rgba pm = alia_rgba_make(0.4f, 0.2f, 0.1f, 0.5f);
        check_near_rgba(
            alia_apply_alpha_rgba(pm, 0.5f),
            alia_rgba_make(0.2f, 0.1f, 0.05f, 0.25f),
            eps);
    }

    /* OKLab <-> RGB (in-gamut) */
    {
        alia_rgb const probes[] = {
            {0.f, 0.f, 0.f},
            {1.f, 1.f, 1.f},
            {1.f, 0.f, 0.f},
            {0.2f, 0.45f, 0.7f},
        };
        for (size_t i = 0; i < sizeof probes / sizeof probes[0]; ++i)
        {
            alia_rgb orig = probes[i];
            alia_oklab lab = alia_oklab_from_rgb(orig);
            alia_rgb back = alia_rgb_from_oklab(lab);
            check_near_rgb(back, orig, 0.0005f);
        }
    }

    /* OKLab <-> OKLCH round-trip */
    {
        alia_oklab const probes[] = {
            {0.5f, 0.0f, 0.0f},
            {0.6f, 0.1f, -0.05f},
            {0.4f, -0.12f, 0.08f},
        };
        for (size_t i = 0; i < sizeof probes / sizeof probes[0]; ++i)
        {
            alia_oklab orig = probes[i];
            alia_oklch lch = alia_oklch_from_oklab(orig);
            alia_oklab back = alia_oklab_from_oklch(lch);
            check_near_f(back.l, orig.l, eps);
            check_near_f(back.a, orig.a, eps);
            check_near_f(back.b, orig.b, eps);
        }
    }

    /* clip: in-gamut OKLab is unchanged */
    {
        alia_oklab lab = alia_oklab_from_rgb(alia_rgb_make(0.35f, 0.5f, 0.65f));
        alia_oklab clipped = alia_oklab_clip_chroma_to_srgb(lab);
        check_near_f(clipped.l, lab.l, eps);
        check_near_f(clipped.a, lab.a, eps);
        check_near_f(clipped.b, lab.b, eps);
    }

    /* OKLCH / OKLab convenience paths vs explicit chain */
    {
        alia_srgb8 s = alia_srgb8_make(0xcc, 0x33, 0x66);
        alia_rgb lin = alia_rgb_from_srgb8(s);
        alia_oklch via_rgb = alia_oklch_from_rgb(lin);
        alia_oklch via_srgb = alia_oklch_from_srgb8(s);
        check_near_f(via_rgb.l, via_srgb.l, eps);
        check_near_f(via_rgb.c, via_srgb.c, eps);
        check_near_f(via_rgb.h, via_srgb.h, eps);

        alia_oklab lab = alia_oklab_from_rgb(lin);
        alia_srgb8 from_lab = alia_srgb8_from_oklab(lab);
        alia_srgb8 from_rgb = alia_srgb8_from_rgb(lin);
        check_srgb8_eq(from_lab, from_rgb);
    }

    /* unclamped OKLCH clamps L and C to [0,1], then matches plain chain */
    {
        alia_oklch extreme = alia_oklch_make(2.0f, 2.0f, 0.5f);
        alia_oklch clamped = alia_oklch_make(1.0f, 1.0f, 0.5f);
        alia_srgb8 via_unclamp = alia_srgb8_from_unclamped_oklch(extreme);
        alia_srgb8 via_chain =
            alia_srgb8_from_oklab(alia_oklab_from_oklch(clamped));
        check_srgb8_eq(via_unclamp, via_chain);
    }
    {
        alia_oklch lch = alia_oklch_make(0.55f, 0.08f, 1.0f);
        check_srgb8_eq(
            alia_srgb8_from_unclamped_oklch(lch),
            alia_srgb8_from_oklab(alia_oklab_from_oklch(lch)));
    }

    /* raw lerps */
    {
        alia_rgb a = alia_rgb_make(0.f, 0.25f, 0.5f);
        alia_rgb b = alia_rgb_make(1.f, 0.75f, 0.25f);
        check_near_rgb(alia_lerp_rgb_raw(a, b, 0.f), a, 0.f);
        check_near_rgb(alia_lerp_rgb_raw(a, b, 1.f), b, 0.f);
        check_near_rgb(
            alia_lerp_rgb_raw(a, b, 0.5f),
            alia_rgb_make(0.5f, 0.5f, 0.375f),
            eps);

        alia_rgba ra = alia_rgba_make(0.f, 1.f, 0.f, 0.25f);
        alia_rgba rb = alia_rgba_make(1.f, 0.f, 1.f, 0.75f);
        check_near_rgba(alia_lerp_rgba_raw(ra, rb, 0.f), ra, 0.f);
        check_near_rgba(alia_lerp_rgba_raw(ra, rb, 1.f), rb, 0.f);
        check_near_rgba(
            alia_lerp_rgba_raw(ra, rb, 0.5f),
            alia_rgba_make(0.5f, 0.5f, 0.5f, 0.5f),
            eps);
    }

    /* OKLCH lerp: same hue -> linear L and C */
    {
        alia_oklch p = alia_oklch_make(0.4f, 0.1f, 0.7f);
        alia_oklch q = alia_oklch_make(0.6f, 0.2f, 0.7f);
        alia_oklch mid = alia_lerp_oklch(p, q, 0.5f);
        check_near_f(mid.l, 0.5f, eps);
        check_near_f(mid.c, 0.15f, eps);
        check_near_f(mid.h, 0.7f, eps);
    }

    /* sRGB8 lerp via OKLCH endpoints */
    {
        alia_srgb8 a = alia_srgb8_make(0xff, 0x00, 0x00);
        alia_srgb8 b = alia_srgb8_make(0x00, 0x00, 0xff);
        check_srgb8_eq(alia_lerp_srgb8_via_oklch(a, b, 0.f), a);
        check_srgb8_eq(alia_lerp_srgb8_via_oklch(a, b, 1.f), b);
    }

    /* sRGB8 round-trip through OKLab (in gamut) */
    {
        alia_srgb8 orig = alia_srgb8_make(0x55, 0xaa, 0x22);
        alia_oklab lab = alia_oklab_from_rgb(alia_rgb_from_srgb8(orig));
        alia_srgb8 back = alia_srgb8_from_oklab(lab);
        check_near_f((float) back.r, (float) orig.r, eps_rt);
        check_near_f((float) back.g, (float) orig.g, eps_rt);
        check_near_f((float) back.b, (float) orig.b, eps_rt);
    }

    /* relative luminance (linear RGB) */
    check_near_f(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8((alia_srgb8){0x00, 0x00, 0x00})),
        0.0000f,
        eps);
    check_near_f(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8((alia_srgb8){0xff, 0xff, 0xff})),
        1.0000f,
        eps);
    check_near_f(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8((alia_srgb8){0xff, 0x00, 0x00})),
        0.2126f,
        eps);
    check_near_f(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8((alia_srgb8){0x00, 0xff, 0x00})),
        0.7152f,
        eps);
    check_near_f(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8((alia_srgb8){0x00, 0x00, 0xff})),
        0.0722f,
        eps);
    check_near_f(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8((alia_srgb8){0x80, 0x80, 0x80})),
        0.2159f,
        eps);
    check_near_f(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8((alia_srgb8){0xff, 0x80, 0x80})),
        0.3826f,
        eps);

    /* RGBA / sRGB8 luminance uses same linear weights on r,g,b */
    {
        alia_rgb lin = alia_rgb_from_srgb8(alia_srgb8_make(0x80, 0x80, 0x80));
        float y_rgb = alia_relative_luminance_rgb(lin);
        float y_rgba = alia_relative_luminance_rgba(
            alia_rgba_make(lin.r, lin.g, lin.b, 1.f));
        check_near_f(y_rgb, y_rgba, 0.f);
        alia_srgb8 s = alia_srgb8_make(0x80, 0x80, 0x80);
        check_near_f(
            alia_relative_luminance_srgb8(s),
            alia_relative_luminance_rgb(alia_rgb_from_srgb8(s)),
            0.f);
    }

    /* contrast ratio: lighter / darker per WCAG-style definition */
    {
        float y_w = 1.f;
        float y_b = 0.f;
        float r1 = alia_relative_luminance_ratio(y_w, y_b);
        float r2 = alia_relative_luminance_ratio(y_b, y_w);
        check_near_f(r1, r2, eps);
        check_near_f(r1, (1.f + 0.05f) / (0.f + 0.05f), eps);
    }
}
