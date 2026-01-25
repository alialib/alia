#include <alia/abi/base/geometry/vec2.h>

#include <doctest/doctest.h>

#include <cmath>

// This file is meant to confirm that the C++ operator macros used in the C API
// are invoking the correct functions.

using namespace alia::operators;

static void
check_vec2i_eq(alia_vec2i a, alia_vec2i b)
{
    CHECK(alia_vec2i_equal(a, b));
}

static void
check_vec2f_near(alia_vec2f a, alia_vec2f b, float eps = 1e-6f)
{
    CHECK(alia_vec2f_near(a, b, eps));
}

TEST_CASE("alia_vec2i operators match C API")
{
    alia_vec2i a = alia_vec2i_make(3, -4);
    alia_vec2i b = alia_vec2i_make(10, 7);

    SUBCASE("operator+")
    {
        alia_vec2i got = a + b;
        alia_vec2i expected = alia_vec2i_add(a, b);
        check_vec2i_eq(got, expected);
    }

    SUBCASE("operator-")
    {
        alia_vec2i got = a - b;
        alia_vec2i expected = alia_vec2i_sub(a, b);
        check_vec2i_eq(got, expected);
    }

    SUBCASE("operator+=")
    {
        alia_vec2i got = a;
        got += b;

        alia_vec2i expected = a;
        alia_vec2i_add_inplace(&expected, b);

        check_vec2i_eq(got, expected);
    }

    SUBCASE("operator-=")
    {
        alia_vec2i got = a;
        got -= b;

        alia_vec2i expected = a;
        alia_vec2i_sub_inplace(&expected, b);

        check_vec2i_eq(got, expected);
    }

    SUBCASE("operator== (if provided)")
    {
        CHECK((a == a));
        CHECK_FALSE((a == b));
    }
}

TEST_CASE("alia_vec2f operators match C API")
{
    alia_vec2f a = alia_vec2f_make(1.25f, -2.5f);
    alia_vec2f b = alia_vec2f_make(-4.0f, 0.75f);

    SUBCASE("operator+")
    {
        alia_vec2f got = a + b;
        alia_vec2f expected = alia_vec2f_add(a, b);
        check_vec2f_near(got, expected);
    }

    SUBCASE("operator-")
    {
        alia_vec2f got = a - b;
        alia_vec2f expected = alia_vec2f_sub(a, b);
        check_vec2f_near(got, expected);
    }

    SUBCASE("operator+=")
    {
        alia_vec2f got = a;
        got += b;

        alia_vec2f expected = a;
        alia_vec2f_add_inplace(&expected, b);

        check_vec2f_near(got, expected);
    }

    SUBCASE("operator-=")
    {
        alia_vec2f got = a;
        got -= b;

        alia_vec2f expected = a;
        alia_vec2f_sub_inplace(&expected, b);

        check_vec2f_near(got, expected);
    }

    SUBCASE("operator*(vec2f, scalar)")
    {
        float s = 2.0f;
        alia_vec2f got = a * s;
        alia_vec2f expected = alia_vec2f_scale(a, s);
        check_vec2f_near(got, expected);
    }

    SUBCASE("operator*(scalar, vec2f) (if provided)")
    {
        float s = -0.5f;
        alia_vec2f got = s * a;
        alia_vec2f expected = alia_vec2f_scale(a, s);
        check_vec2f_near(got, expected);
    }

    SUBCASE("operator*= (if provided)")
    {
        float s = 3.0f;

        alia_vec2f got = a;
        got *= s;

        alia_vec2f expected = a;
        alia_vec2f_scale_inplace(&expected, s);

        check_vec2f_near(got, expected);
    }

    SUBCASE("operator== (if provided)")
    {
        CHECK((a == a));
        CHECK_FALSE((a == b));
    }
}
