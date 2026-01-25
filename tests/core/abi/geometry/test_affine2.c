#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <alia/abi/base/geometry/affine2.h>
#include <alia/abi/base/geometry/box.h>
#include <alia/abi/base/geometry/vec2.h>

static void
check_near_f(float a, float b, float eps)
{
    TEST_CHECK(fabsf(a - b) <= eps);
}

static void
check_vec2_near(alia_vec2f a, alia_vec2f b, float eps)
{
    check_near_f(a.x, b.x, eps);
    check_near_f(a.y, b.y, eps);
}

static void
check_affine_near(alia_affine2 a, alia_affine2 b, float eps)
{
    check_near_f(a.a, b.a, eps);
    check_near_f(a.b, b.b, eps);
    check_near_f(a.c, b.c, eps);
    check_near_f(a.d, b.d, eps);
    check_near_f(a.tx, b.tx, eps);
    check_near_f(a.ty, b.ty, eps);
}

static void
test_identity(void)
{
    alia_affine2 I = alia_affine2_identity();

    alia_vec2f p = alia_vec2f_make(3.0f, -4.0f);
    check_vec2_near(alia_affine2_transform_point(I, p), p, 0.0f);

    alia_vec2f v = alia_vec2f_make(1.5f, 2.5f);
    check_vec2_near(alia_affine2_transform_vector(I, v), v, 0.0f);
}

static void
test_translation(void)
{
    alia_affine2 T = alia_affine2_translation(10.0f, -2.0f);

    alia_vec2f p = alia_vec2f_make(1.0f, 2.0f);
    alia_vec2f p2 = alia_affine2_transform_point(T, p);
    check_vec2_near(p2, alia_vec2f_make(11.0f, 0.0f), 0.0f);

    // vectors should not be affected by translation
    alia_vec2f v = alia_vec2f_make(1.0f, 2.0f);
    alia_vec2f v2 = alia_affine2_transform_vector(T, v);
    check_vec2_near(v2, v, 0.0f);
}

static void
test_scaling(void)
{
    alia_affine2 S = alia_affine2_scaling(2.0f, -3.0f);

    alia_vec2f p = alia_vec2f_make(1.5f, -2.0f);
    check_vec2_near(
        alia_affine2_transform_point(S, p), alia_vec2f_make(3.0f, 6.0f), 0.0f);

    alia_vec2f v = alia_vec2f_make(1.5f, -2.0f);
    check_vec2_near(
        alia_affine2_transform_vector(S, v),
        alia_vec2f_make(3.0f, 6.0f),
        0.0f);
}

static void
test_rotation_90deg(void)
{
    // r = +pi/2: (x,y) -> (-y, x)
    alia_affine2 R = alia_affine2_rotation(0.5f * 3.14159265358979323846f);

    alia_vec2f p = alia_vec2f_make(2.0f, 3.0f);
    alia_vec2f p2 = alia_affine2_transform_point(R, p);

    // allow small epsilon due to sin/cos
    check_vec2_near(p2, alia_vec2f_make(-3.0f, 2.0f), 1e-5f);
}

static void
test_compose_matches_sequential_application(void)
{
    // Apply child then parent: p(c(x))
    alia_affine2 C = alia_affine2_translation(5.0f, 7.0f);
    alia_affine2 P = alia_affine2_scaling(2.0f, 3.0f);

    alia_affine2 PC = alia_affine2_compose(P, C);

    alia_vec2f x = alia_vec2f_make(1.0f, 2.0f);

    alia_vec2f seq
        = alia_affine2_transform_point(P, alia_affine2_transform_point(C, x));
    alia_vec2f one = alia_affine2_transform_point(PC, x);

    check_vec2_near(one, seq, 0.0f);
}

static void
test_invert(void)
{
    // A general-ish invertible transform: scale then translate then rotate
    alia_affine2 S = alia_affine2_scaling(2.0f, 0.5f);
    alia_affine2 R = alia_affine2_rotation(0.3f);
    alia_affine2 T = alia_affine2_translation(10.0f, -4.0f);

    // M = T * R * S
    alia_affine2 M = alia_affine2_compose(T, alia_affine2_compose(R, S));
    alia_affine2 Inv = alia_affine2_invert(M);

    // M * Inv ≈ I
    alia_affine2 I = alia_affine2_compose(M, Inv);
    check_affine_near(I, alia_affine2_identity(), 1e-5f);

    // point round-trip
    alia_vec2f p = alia_vec2f_make(-3.0f, 8.0f);
    alia_vec2f p2 = alia_affine2_transform_point(M, p);
    alia_vec2f p3 = alia_affine2_transform_point(Inv, p2);
    check_vec2_near(p3, p, 1e-5f);

    // vector round-trip (no translation anyway)
    alia_vec2f v = alia_vec2f_make(1.0f, -2.0f);
    alia_vec2f v2 = alia_affine2_transform_vector(M, v);
    alia_vec2f v3 = alia_affine2_transform_vector(Inv, v2);
    check_vec2_near(v3, v, 1e-5f);
}

static void
test_transform_aabb_translation(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(1.0f, 2.0f), alia_vec2f_make(4.0f, 6.0f));
    alia_affine2 T = alia_affine2_translation(10.0f, -3.0f);

    alia_box b2 = alia_affine2_transform_aabb(T, b);

    check_vec2_near(b2.min, alia_vec2f_make(11.0f, -1.0f), 0.0f);
    check_vec2_near(b2.size, b.size, 0.0f);
}

static void
test_transform_aabb_scaling(void)
{
    alia_box b = alia_box_make(
        alia_vec2f_make(-2.0f, 1.0f), alia_vec2f_make(3.0f, 5.0f));
    alia_affine2 S = alia_affine2_scaling(2.0f, 3.0f);

    alia_box b2 = alia_affine2_transform_aabb(S, b);

    check_vec2_near(b2.min, alia_vec2f_make(-4.0f, 3.0f), 0.0f);
    check_vec2_near(b2.size, alia_vec2f_make(6.0f, 15.0f), 0.0f);
}

static void
test_transform_aabb_rotation_90deg_about_origin(void)
{
    // AABB from (0,0) size (2,4): corners (0,0),(2,0),(0,4),(2,4)
    alia_box b = alia_box_make(
        alia_vec2f_make(0.0f, 0.0f), alia_vec2f_make(2.0f, 4.0f));
    alia_affine2 R = alia_affine2_rotation(0.5f * 3.14159265358979323846f);

    alia_box b2 = alia_affine2_transform_aabb(R, b);

    // Rotated points:
    // (0,0)->(0,0)
    // (2,0)->(0,2)
    // (0,4)->(-4,0)
    // (2,4)->(-4,2)
    // AABB min (-4,0), max (0,2) => size (4,2)
    check_vec2_near(b2.min, alia_vec2f_make(-4.0f, 0.0f), 1e-5f);
    check_vec2_near(b2.size, alia_vec2f_make(4.0f, 2.0f), 1e-5f);
}

void
affine2_tests(void)
{
    test_identity();
    test_translation();
    test_scaling();
    test_rotation_90deg();
    test_compose_matches_sequential_application();
    test_invert();
    test_transform_aabb_translation();
    test_transform_aabb_scaling();
    test_transform_aabb_rotation_90deg_about_origin();
}
