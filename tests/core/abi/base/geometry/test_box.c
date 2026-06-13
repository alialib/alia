#include <alia/abi/base/geometry/box.h>

#include <alia/abi/base/geometry/edge_offsets.h>

#define TEST_NO_MAIN
#include "acutest.h"

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
    alia_edge_offsets offsets
        = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f);

    alia_box out = alia_box_expand(b, offsets);

    // min decreases by (left, top)
    // size increases by (left+right, top+bottom)
    TEST_CHECK(alia_box_equal(
        out,
        alia_box_make(
            alia_vec2f_make(6.f, 19.f), alia_vec2f_make(36.f, 44.f))));

    // Insetting back should return the original.
    alia_box roundtrip = alia_box_shrink(out, offsets);
    TEST_CHECK(alia_box_equal(roundtrip, b));
}

static void
test_shrink_and_expand_roundtrip(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));
    alia_edge_offsets offsets
        = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f);

    alia_box in = alia_box_shrink(b, offsets);

    // min increases by (left, top)
    // size decreases by (left+right, top+bottom)
    TEST_CHECK(alia_box_equal(
        in,
        alia_box_make(
            alia_vec2f_make(14.f, 21.f), alia_vec2f_make(24.f, 36.f))));

    // Outsetting back should return the original.
    alia_box roundtrip = alia_box_expand(in, offsets);
    TEST_CHECK(alia_box_equal(roundtrip, b));
}

static void
test_union(void)
{
    alia_box a = alia_box_make(
        alia_vec2f_make(0.f, 0.f), alia_vec2f_make(10.f, 10.f));
    alia_box b = alia_box_make(
        alia_vec2f_make(5.f, 5.f), alia_vec2f_make(10.f, 10.f));

    TEST_CHECK(alia_box_equal(
        alia_box_union(a, b),
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(15.f, 15.f))));

    // One box contained in the other returns the enclosing box.
    alia_box outer = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(30.f, 40.f));
    alia_box inner = alia_box_make(
        alia_vec2f_make(12.f, 22.f), alia_vec2f_make(5.f, 5.f));
    TEST_CHECK(alia_box_equal(alia_box_union(outer, inner), outer));
    TEST_CHECK(alia_box_equal(alia_box_union(inner, outer), outer));

    // Disjoint boxes return the minimal enclosing box.
    alia_box left
        = alia_box_make(alia_vec2f_make(0.f, 0.f), alia_vec2f_make(5.f, 5.f));
    alia_box right = alia_box_make(
        alia_vec2f_make(10.f, 10.f), alia_vec2f_make(5.f, 5.f));
    TEST_CHECK(alia_box_equal(
        alia_box_union(left, right),
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(15.f, 15.f))));

    // Union with itself is unchanged.
    TEST_CHECK(alia_box_equal(alia_box_union(a, a), a));
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
    test_union();
    test_contains_edges();
}
