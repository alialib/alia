#include <alia/abi/ui/layout/popup.h>

#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

static alia_box
box(float x, float y, float w, float h)
{
    return alia_box_make(alia_vec2f_make(x, y), alia_vec2f_make(w, h));
}

static void
check_box(alia_box actual, float x, float y, float w, float h)
{
    TEST_CHECK(fabsf(actual.min.x - x) < 0.001f);
    TEST_CHECK(fabsf(actual.min.y - y) < 0.001f);
    TEST_CHECK(fabsf(actual.size.x - w) < 0.001f);
    TEST_CHECK(fabsf(actual.size.y - h) < 0.001f);
}

static alia_box const surface = {{0.f, 0.f}, {400.f, 300.f}};

static void
test_prefers_below_start_when_it_fits(void)
{
    alia_box const placed = alia_popup_place(
        box(100.f, 100.f, 80.f, 20.f),
        alia_vec2f_make(60.f, 40.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 100.f, 120.f, 60.f, 40.f);
}

static void
test_flips_to_end_when_start_cannot_cover_the_width(void)
{
    alia_box const placed = alia_popup_place(
        box(300.f, 100.f, 80.f, 20.f),
        alia_vec2f_make(150.f, 40.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 230.f, 120.f, 150.f, 40.f);
}

static void
test_flips_above_when_below_covers_less_of_the_height(void)
{
    alia_box const placed = alia_popup_place(
        box(50.f, 250.f, 40.f, 20.f),
        alia_vec2f_make(30.f, 80.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 50.f, 170.f, 30.f, 80.f);
}

static void
test_keeps_below_start_when_both_sides_fit(void)
{
    alia_box const placed = alia_popup_place(
        box(40.f, 40.f, 50.f, 20.f),
        alia_vec2f_make(30.f, 30.f),
        alia_vec2f_make(0.f, 0.f),
        surface);
    check_box(placed, 40.f, 60.f, 30.f, 30.f);
}

static void
test_clamps_to_the_largest_side_when_nothing_fits(void)
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

static void
test_raises_the_size_to_the_minimum_and_still_clamps_it(void)
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

void
popup_tests(void)
{
    test_prefers_below_start_when_it_fits();
    test_flips_to_end_when_start_cannot_cover_the_width();
    test_flips_above_when_below_covers_less_of_the_height();
    test_keeps_below_start_when_both_sides_fit();
    test_clamps_to_the_largest_side_when_nothing_fits();
    test_raises_the_size_to_the_minimum_and_still_clamps_it();
}
