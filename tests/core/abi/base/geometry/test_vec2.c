#include <alia/abi/base/geometry/vec2.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <math.h>

static void
test_constructors_and_equality(void)
{
    alia_vec2i ai = alia_vec2i_make(1, -2);
    TEST_CHECK(ai.x == 1);
    TEST_CHECK(ai.y == -2);

    alia_vec2f af = alia_vec2f_make(1.5f, -2.25f);
    TEST_CHECK(af.x == 1.5f);
    TEST_CHECK(af.y == -2.25f);

    TEST_CHECK(alia_vec2i_equal(alia_vec2i_make(3, 4), alia_vec2i_make(3, 4)));
    TEST_CHECK(
        !alia_vec2i_equal(alia_vec2i_make(3, 4), alia_vec2i_make(3, 5)));

    TEST_CHECK(
        alia_vec2f_equal(alia_vec2f_make(3.0f, 4.0f), alia_vec2f_make(3.0f, 4.0f)));
    TEST_CHECK(
        !alia_vec2f_equal(
            alia_vec2f_make(3.0f, 4.0f), alia_vec2f_make(3.0f, 4.0001f)));
}

static void
test_conversions(void)
{
    alia_vec2i vi = alia_vec2i_make(12, -7);
    alia_vec2f vf = alia_vec2i_to_vec2f(vi);
    TEST_CHECK(vf.x == 12.0f);
    TEST_CHECK(vf.y == -7.0f);

    /* truncation toward zero */
    {
        alia_vec2f a = alia_vec2f_make(2.9f, -2.9f);
        alia_vec2i at = alia_vec2f_to_vec2i_trunc(a);
        TEST_CHECK(at.x == 2);
        TEST_CHECK(at.y == -2);
    }

    /* floor/ceil/round: include negatives */
    {
        alia_vec2f b = alia_vec2f_make(2.2f, -2.2f);

        alia_vec2i bf = alia_vec2f_floor(b);
        TEST_CHECK(bf.x == 2);
        TEST_CHECK(bf.y == -3);

        alia_vec2i bc = alia_vec2f_ceil(b);
        TEST_CHECK(bc.x == 3);
        TEST_CHECK(bc.y == -2);

        alia_vec2i br = alia_vec2f_round(b);
        TEST_CHECK(br.x == 2);
        TEST_CHECK(br.y == -2);
    }
}

static void
test_vec2f_arithmetic(void)
{
    alia_vec2f a = alia_vec2f_make(1.0f, 2.0f);
    alia_vec2f b = alia_vec2f_make(3.0f, -4.0f);

    {
        alia_vec2f s = alia_vec2f_add(a, b);
        TEST_CHECK(s.x == 4.0f);
        TEST_CHECK(s.y == -2.0f);
    }
    {
        alia_vec2f d = alia_vec2f_sub(a, b);
        TEST_CHECK(d.x == -2.0f);
        TEST_CHECK(d.y == 6.0f);
    }
    {
        alia_vec2f m = alia_vec2f_scale(b, 2.0f);
        TEST_CHECK(m.x == 6.0f);
        TEST_CHECK(m.y == -8.0f);
    }
    {
        float dot = alia_vec2f_dot(a, b);
        TEST_CHECK(dot == -5.0f); /* 1*3 + 2*(-4) */
    }
    {
        float lsq = alia_vec2f_length_sq(alia_vec2f_make(3.0f, 4.0f));
        TEST_CHECK(lsq == 25.0f);
    }
    {
        float len = alia_vec2f_length(alia_vec2f_make(3.0f, 4.0f));
        TEST_CHECK(fabsf(len - 5.0f) < 1e-6f);
    }
}

static void
test_vec2i_arithmetic(void)
{
    alia_vec2i a = alia_vec2i_make(10, 20);
    alia_vec2i b = alia_vec2i_make(-2, 5);

    {
        alia_vec2i s = alia_vec2i_add(a, b);
        TEST_CHECK(s.x == 8);
        TEST_CHECK(s.y == 25);
    }
    {
        alia_vec2i d = alia_vec2i_sub(a, b);
        TEST_CHECK(d.x == 12);
        TEST_CHECK(d.y == 15);
    }
    {
        alia_vec2i mn = alia_vec2i_min(a, b);
        TEST_CHECK(mn.x == -2);
        TEST_CHECK(mn.y == 5);
    }
    {
        alia_vec2i mx = alia_vec2i_max(a, b);
        TEST_CHECK(mx.x == 10);
        TEST_CHECK(mx.y == 20);
    }
}

void
vec2_tests(void)
{
    test_constructors_and_equality();
    test_conversions();
    test_vec2f_arithmetic();
    test_vec2i_arithmetic();
}

