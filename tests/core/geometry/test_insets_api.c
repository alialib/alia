#include <alia/abi/geometry/insets_api.h>

#define TEST_NO_MAIN
#include "acutest.h"

static void
check_insets(alia_insets i, float left, float right, float top, float bottom)
{
    TEST_CHECK(i.left == left);
    TEST_CHECK(i.right == right);
    TEST_CHECK(i.top == top);
    TEST_CHECK(i.bottom == bottom);
}

static void
test_make_trbl(void)
{
    alia_insets i = alia_insets_make_trbl(1.f, 2.f, 3.f, 4.f);
    // trbl args: top=1, right=2, bottom=3, left=4
    check_insets(i, 4.f, 2.f, 1.f, 3.f);
}

static void
test_make_xy(void)
{
    alia_insets i = alia_insets_make_xy(10.f, 20.f);
    check_insets(i, 10.f, 10.f, 20.f, 20.f);
}

static void
test_make_uniform(void)
{
    alia_insets i = alia_insets_make_uniform(7.f);
    check_insets(i, 7.f, 7.f, 7.f, 7.f);
}

static void
test_equal(void)
{
    alia_insets a = alia_insets_make_trbl(1.f, 2.f, 3.f, 4.f);
    alia_insets b = alia_insets_make_trbl(1.f, 2.f, 3.f, 4.f);
    alia_insets c = alia_insets_make_trbl(1.f, 2.f, 3.f, 5.f);

    TEST_CHECK(alia_insets_equal(a, b));
    TEST_CHECK(!alia_insets_equal(a, c));
}

static void
test_add_and_inplace(void)
{
    alia_insets a
        = alia_insets_make_trbl(1.f, 2.f, 3.f, 4.f); // L=4 R=2 T=1 B=3
    alia_insets b
        = alia_insets_make_trbl(10.f, 20.f, 30.f, 40.f); // L=40 R=20 T=10 B=30

    alia_insets c = alia_insets_add(a, b);
    check_insets(c, 44.f, 22.f, 11.f, 33.f);

    alia_insets_add_inplace(&a, b);
    check_insets(a, 44.f, 22.f, 11.f, 33.f);
}

static void
test_sub_and_inplace(void)
{
    alia_insets a
        = alia_insets_make_trbl(5.f, 6.f, 7.f, 8.f); // L=8 R=6 T=5 B=7
    alia_insets b
        = alia_insets_make_trbl(1.f, 2.f, 3.f, 4.f); // L=4 R=2 T=1 B=3

    alia_insets c = alia_insets_sub(a, b);
    check_insets(c, 4.f, 4.f, 4.f, 4.f);

    alia_insets_sub_inplace(&a, b);
    check_insets(a, 4.f, 4.f, 4.f, 4.f);
}

static void
test_scale_and_inplace(void)
{
    alia_insets a
        = alia_insets_make_trbl(1.5f, 2.0f, 2.5f, 3.0f); // L=3 R=2 T=1.5 B=2.5

    alia_insets c = alia_insets_scale(a, 2.f);
    check_insets(c, 6.f, 4.f, 3.f, 5.f);

    alia_insets_scale_inplace(&a, 2.f);
    check_insets(a, 6.f, 4.f, 3.f, 5.f);
}

void
insets_tests(void)
{
    test_make_trbl();
    test_make_xy();
    test_make_uniform();
    test_equal();
    test_add_and_inplace();
    test_sub_and_inplace();
    test_scale_and_inplace();
}
