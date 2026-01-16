#include <alia/abi/geometry/box_api.h>

#define TEST_NO_MAIN
#include "acutest.h"

// Helper: build an insets value (assuming alia_insets has
// left/top/right/bottom float fields).
static inline alia_insets
make_insets(float left, float top, float right, float bottom)
{
    alia_insets i;
    i.left = left;
    i.top = top;
    i.right = right;
    i.bottom = bottom;
    return i;
}

static void
test_make_and_equal(void)
{
    alia_box a
        = alia_box_make(alia_vec2f_make(1.f, 2.f), alia_vec2f_make(3.f, 4.f));
    alia_box b
        = alia_box_make(alia_vec2f_make(1.f, 2.f), alia_vec2f_make(3.f, 4.f));
    alia_box c
        = alia_box_make(alia_vec2f_make(1.f, 2.f), alia_vec2f_make(3.f, 5.f));

    TEST_CHECK(alia_box_equal(a, b));
    TEST_CHECK(!alia_box_equal(a, c));
}

static void
test_expand_and_shrink_roundtrip(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));
    alia_vec2f v = alia_vec2f_make(2.f, 3.f);

    alia_box expanded = alia_box_expand(b, v);

    // min decreases by v; size increases by 2*v
    TEST_CHECK(alia_box_equal(
        expanded,
        alia_box_make(
            alia_vec2f_make(8.f, 17.f), alia_vec2f_make(34.f, 46.f))));

    // Shrinking back by the same amount should return the original.
    alia_box roundtrip = alia_box_shrink(expanded, v);
    TEST_CHECK(alia_box_equal(roundtrip, b));
}

static void
test_shrink_and_expand_roundtrip(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));
    alia_vec2f v = alia_vec2f_make(2.f, 3.f);

    alia_box shrunk = alia_box_shrink(b, v);

    // min increases by v; size decreases by 2*v
    TEST_CHECK(alia_box_equal(
        shrunk,
        alia_box_make(
            alia_vec2f_make(12.f, 23.f), alia_vec2f_make(26.f, 34.f))));

    // Expanding back should return the original.
    alia_box roundtrip = alia_box_expand(shrunk, v);
    TEST_CHECK(alia_box_equal(roundtrip, b));
}

static void
test_outset_and_inset_roundtrip(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));
    alia_insets i = make_insets(1.f, 2.f, 3.f, 4.f);

    alia_box out = alia_box_outset(b, i);

    // min decreases by (left, top)
    // size increases by (left+right, top+bottom)
    TEST_CHECK(alia_box_equal(
        out,
        alia_box_make(
            alia_vec2f_make(9.f, 18.f), alia_vec2f_make(34.f, 46.f))));

    // Insetting back should return the original.
    alia_box roundtrip = alia_box_inset(out, i);
    TEST_CHECK(alia_box_equal(roundtrip, b));
}

static void
test_inset_and_outset_roundtrip(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));
    alia_insets i = make_insets(1.f, 2.f, 3.f, 4.f);

    alia_box in = alia_box_inset(b, i);

    // min increases by (left, top)
    // size decreases by (left+right, top+bottom)
    TEST_CHECK(alia_box_equal(
        in,
        alia_box_make(
            alia_vec2f_make(11.f, 22.f), alia_vec2f_make(26.f, 34.f))));

    // Outsetting back should return the original.
    alia_box roundtrip = alia_box_outset(in, i);
    TEST_CHECK(alia_box_equal(roundtrip, b));
}

static void
test_outset_matches_expand_for_uniform_amount(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));
    alia_vec2f v = alia_vec2f_make(2.f, 3.f);

    // Outset with uniform insets should match expand(v)
    alia_insets uniform = make_insets(2.f, 3.f, 2.f, 3.f);

    alia_box a = alia_box_expand(b, v);
    alia_box o = alia_box_outset(b, uniform);

    TEST_CHECK(alia_box_equal(a, o));
}

static void
test_contains_edges(void)
{
    // box from (10,20) with size (30,40) covers:
    // x in [10, 40) and y in [20, 60)
    alia_box b = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));

    // Min corner is inside (inclusive)
    TEST_CHECK(alia_box_contains(b, alia_vec2f_make(10.f, 20.f)));

    // Just inside max edge
    TEST_CHECK(alia_box_contains(b, alia_vec2f_make(39.f, 59.f)));

    // Max edges are outside (exclusive)
    TEST_CHECK(!alia_box_contains(b, alia_vec2f_make(40.f, 20.f)));
    TEST_CHECK(!alia_box_contains(b, alia_vec2f_make(10.f, 60.f)));
    TEST_CHECK(!alia_box_contains(b, alia_vec2f_make(40.f, 60.f)));

    // Outside
    TEST_CHECK(!alia_box_contains(b, alia_vec2f_make(9.f, 20.f)));
    TEST_CHECK(!alia_box_contains(b, alia_vec2f_make(10.f, 19.f)));
    TEST_CHECK(!alia_box_contains(b, alia_vec2f_make(41.f, 20.f)));
    TEST_CHECK(!alia_box_contains(b, alia_vec2f_make(10.f, 61.f)));
}

void
box_tests()
{
    test_make_and_equal();
    test_expand_and_shrink_roundtrip();
    test_shrink_and_expand_roundtrip();
    test_outset_and_inset_roundtrip();
    test_inset_and_outset_roundtrip();
    test_outset_matches_expand_for_uniform_amount();
    test_contains_edges();
}
