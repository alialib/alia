#include <alia/abi/base/geometry/box.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/utilities/placement.h>

#include <doctest/doctest.h>

TEST_CASE("alia_resolve_growth_factor")
{
    CHECK(alia_resolve_growth_factor(0) == 0.f);
    CHECK(alia_resolve_growth_factor(ALIA_GROW) == 1.f);
}

TEST_CASE("alia_resolve_leaf_box fill")
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_FILL,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(0.f, 0.f));

    CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f))));
}

TEST_CASE("alia_resolve_leaf_box corner alignment")
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_ALIGN_RIGHT | ALIA_ALIGN_BOTTOM,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(0.f, 0.f));

    CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(90.f, 90.f), alia_vec2f_make(10.f, 10.f))));
}

TEST_CASE("alia_resolve_leaf_box axis fill")
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_FILL_X | ALIA_ALIGN_TOP,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(0.f, 0.f));

    CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 10.f))));
}

TEST_CASE("alia_resolve_container_box default stretch")
{
    alia_box box = alia_resolve_container_box(
        0,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f);

    CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f))));
}

TEST_CASE("alia_resolve_container_box fill")
{
    alia_box box = alia_resolve_container_box(
        ALIA_FILL,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f);

    CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f))));
}

TEST_CASE("alia_resolve_container_box centered")
{
    alia_box box = alia_resolve_container_box(
        ALIA_CENTER,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f);

    CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(45.f, 45.f), alia_vec2f_make(10.f, 10.f))));
}
