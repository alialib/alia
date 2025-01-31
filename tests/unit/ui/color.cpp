#include <alia/ui/color.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace alia;
using namespace Catch::Matchers;

TEST_CASE("relative luminance")
{
    CHECK_THAT(
        calculate_relative_luminance(rgb8(0x00, 0x00, 0x00)),
        WithinAbs(0.0000f, 0.0001f));
    CHECK_THAT(
        calculate_relative_luminance(rgb8(0xff, 0xff, 0xff)),
        WithinAbs(1.0000f, 0.0001f));
    CHECK_THAT(
        calculate_relative_luminance(rgb8(0xff, 0x00, 0x00)),
        WithinAbs(0.2126f, 0.0001f));
    CHECK_THAT(
        calculate_relative_luminance(rgb8(0x00, 0xff, 0x00)),
        WithinAbs(0.7152f, 0.0001f));
    CHECK_THAT(
        calculate_relative_luminance(rgb8(0x00, 0x00, 0xff)),
        WithinAbs(0.0722f, 0.0001f));
    CHECK_THAT(
        calculate_relative_luminance(rgb8(0x80, 0x80, 0x80)),
        WithinAbs(0.2159f, 0.0001f));
    CHECK_THAT(
        calculate_relative_luminance(rgb8(0xff, 0x80, 0x80)),
        WithinAbs(0.3826f, 0.0001f));
}
