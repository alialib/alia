#include <alia/color.h>

#include <doctest/doctest.h>

TEST_CASE("relative luminance")
{
    CHECK(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8(alia_srgb8{0x00, 0x00, 0x00}))
        == doctest::Approx(0.0000f).epsilon(0.0001f));
    CHECK(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8(alia_srgb8{0xff, 0xff, 0xff}))
        == doctest::Approx(1.0000f).epsilon(0.0001f));
    CHECK(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8(alia_srgb8{0xff, 0x00, 0x00}))
        == doctest::Approx(0.2126f).epsilon(0.0001f));
    CHECK(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8(alia_srgb8{0x00, 0xff, 0x00}))
        == doctest::Approx(0.7152f).epsilon(0.0001f));
    CHECK(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8(alia_srgb8{0x00, 0x00, 0xff}))
        == doctest::Approx(0.0722f).epsilon(0.0001f));
    CHECK(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8(alia_srgb8{0x80, 0x80, 0x80}))
        == doctest::Approx(0.2159f).epsilon(0.0001f));
    CHECK(
        alia_relative_luminance_rgb(
            alia_rgb_from_srgb8(alia_srgb8{0xff, 0x80, 0x80}))
        == doctest::Approx(0.3826f).epsilon(0.0001f));
}
