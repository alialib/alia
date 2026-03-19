#include <alia/abi/base/color.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <math.h>

static void
check_near_f(float got, float expected, float eps)
{
    TEST_CHECK(fabsf(got - expected) <= eps);
}

void
color_tests(void)
{
    float eps = 0.0001f;

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
}

