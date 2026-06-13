#include <alia/abi/base/geometry/edge_offsets.h>

#define TEST_NO_MAIN
#include "acutest.h"

static void
check_edge_offsets(
    alia_edge_offsets i, float left, float right, float top, float bottom)
{
    TEST_CHECK(i.left == left);
    TEST_CHECK(i.right == right);
    TEST_CHECK(i.top == top);
    TEST_CHECK(i.bottom == bottom);
}

static void
test_make_trbl(void)
{
    alia_edge_offsets i = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f);
    // trbl args: top=1, right=2, bottom=3, left=4
    check_edge_offsets(i, 4.f, 2.f, 1.f, 3.f);
}

static void
test_make_xy(void)
{
    alia_edge_offsets i = alia_edge_offsets_make_xy(10.f, 20.f);
    check_edge_offsets(i, 10.f, 10.f, 20.f, 20.f);
}

static void
test_make_uniform(void)
{
    alia_edge_offsets i = alia_edge_offsets_make_uniform(7.f);
    check_edge_offsets(i, 7.f, 7.f, 7.f, 7.f);
}

static void
test_equal(void)
{
    alia_edge_offsets a = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f);
    alia_edge_offsets b = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f);
    alia_edge_offsets c = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 5.f);

    TEST_CHECK(alia_edge_offsets_equal(a, b));
    TEST_CHECK(!alia_edge_offsets_equal(a, c));
}

static void
test_add_and_inplace(void)
{
    alia_edge_offsets a
        = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f); // L=4 R=2 T=1 B=3
    alia_edge_offsets b = alia_edge_offsets_make_trbl(
        10.f, 20.f, 30.f, 40.f); // L=40 R=20 T=10 B=30

    alia_edge_offsets c = alia_edge_offsets_add(a, b);
    check_edge_offsets(c, 44.f, 22.f, 11.f, 33.f);

    alia_edge_offsets_add_inplace(&a, b);
    check_edge_offsets(a, 44.f, 22.f, 11.f, 33.f);
}

static void
test_sub_and_inplace(void)
{
    alia_edge_offsets a
        = alia_edge_offsets_make_trbl(5.f, 6.f, 7.f, 8.f); // L=8 R=6 T=5 B=7
    alia_edge_offsets b
        = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f); // L=4 R=2 T=1 B=3

    alia_edge_offsets c = alia_edge_offsets_sub(a, b);
    check_edge_offsets(c, 4.f, 4.f, 4.f, 4.f);

    alia_edge_offsets_sub_inplace(&a, b);
    check_edge_offsets(a, 4.f, 4.f, 4.f, 4.f);
}

static void
test_scale_and_inplace(void)
{
    alia_edge_offsets a = alia_edge_offsets_make_trbl(
        1.5f, 2.0f, 2.5f, 3.0f); // L=3 R=2 T=1.5 B=2.5

    alia_edge_offsets c = alia_edge_offsets_scale(a, 2.f);
    check_edge_offsets(c, 6.f, 4.f, 3.f, 5.f);

    alia_edge_offsets_scale_inplace(&a, 2.f);
    check_edge_offsets(a, 6.f, 4.f, 3.f, 5.f);
}

static void
test_invert_and_inplace(void)
{
    alia_edge_offsets a = alia_edge_offsets_make_trbl(1.f, 2.f, 3.f, 4.f);

    alia_edge_offsets c = alia_edge_offsets_invert(a);
    check_edge_offsets(c, -1.f, -2.f, -3.f, -4.f);

    alia_edge_offsets_invert_inplace(&a);
    check_edge_offsets(a, -1.f, -2.f, -3.f, -4.f);
}

void
edge_offsets_tests(void)
{
    test_make_trbl();
    test_make_xy();
    test_make_uniform();
    test_equal();
    test_add_and_inplace();
    test_sub_and_inplace();
    test_scale_and_inplace();
    test_invert_and_inplace();
}
