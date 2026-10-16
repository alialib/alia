#include <alia/abi/ui/layout/popup.h>

#include <doctest/doctest.h>

namespace {

alia_box
box(float x, float y, float w, float h)
{
    return alia_box_make(alia_vec2f_make(x, y), alia_vec2f_make(w, h));
}

void
check_box(alia_box actual, float x, float y, float w, float h)
{
    CHECK(actual.min.x == doctest::Approx(x));
    CHECK(actual.min.y == doctest::Approx(y));
    CHECK(actual.size.x == doctest::Approx(w));
    CHECK(actual.size.y == doctest::Approx(h));
}

alia_box const surface = box(0.f, 0.f, 400.f, 300.f);

} // namespace

TEST_CASE("popup place prefers below-start when it fits")
{
    alia_box const placed = alia_popup_place(
        box(100.f, 100.f, 80.f, 20.f),
        alia_vec2f_make(60.f, 40.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 100.f, 120.f, 60.f, 40.f);
}

TEST_CASE("popup place flips to end when start cannot cover the width")
{
    alia_box const placed = alia_popup_place(
        box(300.f, 100.f, 80.f, 20.f),
        alia_vec2f_make(150.f, 40.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 230.f, 120.f, 150.f, 40.f);
}

TEST_CASE("popup place flips above when below covers less of the height")
{
    alia_box const placed = alia_popup_place(
        box(50.f, 250.f, 40.f, 20.f),
        alia_vec2f_make(30.f, 80.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 50.f, 170.f, 30.f, 80.f);
}

TEST_CASE("popup place keeps below-start when both sides fit")
{
    alia_box const placed = alia_popup_place(
        box(40.f, 40.f, 50.f, 20.f),
        alia_vec2f_make(30.f, 30.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 40.f, 60.f, 30.f, 30.f);
}

TEST_CASE("popup place clamps to the largest side when nothing fits")
{
    alia_box const placed = alia_popup_place(
        box(10.f, 80.f, 20.f, 20.f),
        alia_vec2f_make(100.f, 100.f),
        alia_vec2f_make(0.f, 0.f),
        box(0.f, 0.f, 200.f, 140.f));
    // Below has 40px. Above has 80px. Start has 190px, so above-start wins
    // and the height is clamped to the space above the anchor.
    check_box(placed, 10.f, 0.f, 100.f, 80.f);
}

TEST_CASE("popup place raises the size to the minimum and still clamps it")
{
    alia_box const placed = alia_popup_place(
        box(100.f, 40.f, 80.f, 20.f),
        alia_vec2f_make(20.f, 10.f),
        alia_vec2f_make(80.f, 50.f),
        surface);
    check_box(placed, 100.f, 60.f, 80.f, 50.f);

    alia_box const clamped = alia_popup_place(
        box(350.f, 40.f, 40.f, 20.f),
        alia_vec2f_make(10.f, 10.f),
        alia_vec2f_make(200.f, 0.f),
        surface);
    // Preferred fits on both sides, so below-start wins. The width floor is
    // then clamped to the 50px available on that side.
    check_box(clamped, 350.f, 60.f, 50.f, 10.f);
}
