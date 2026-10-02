#include <alia/abi/base/geometry/box.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/utilities/placement.h>

#define TEST_NO_MAIN
#include "acutest.h"

static void
test_resolve_growth_factor(void)
{
    TEST_CHECK(alia_resolve_growth_factor(0) == 0.f);
    TEST_CHECK(alia_resolve_growth_factor(ALIA_GROW) == 1.f);
}

static void
test_resolve_leaf_box_fill(void)
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_FILL,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(0.f, 0.f));

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f))));
}

static void
test_resolve_leaf_box_corner_alignment(void)
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_ALIGN_RIGHT | ALIA_ALIGN_BOTTOM,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(0.f, 0.f));

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(90.f, 90.f), alia_vec2f_make(10.f, 10.f))));
}

static void
test_resolve_leaf_box_axis_fill(void)
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_FILL_X | ALIA_ALIGN_TOP,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(0.f, 0.f));

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 10.f))));
}

static void
test_resolve_leaf_box_fill_y_align_right(void)
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_FILL_Y | ALIA_ALIGN_RIGHT,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(0.f, 0.f));

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(90.f, 0.f), alia_vec2f_make(10.f, 100.f))));
}

static void
test_resolve_leaf_box_with_spacing(void)
{
    alia_box box = alia_resolve_leaf_box(
        ALIA_FILL,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f,
        alia_vec2f_make(8.f, 8.f));

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(8.f, 8.f), alia_vec2f_make(84.f, 84.f))));
}

static void
test_resolve_baseline(void)
{
    float baseline = alia_resolve_baseline(
        ALIA_BASELINE_GROUP_ALIGN_CENTER, 100.f, 20.f, 30.f);

    TEST_CHECK(baseline == 45.f);
}

static void
test_resolve_container_box_default_stretch(void)
{
    alia_box box = alia_resolve_container_box(
        0,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f);

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f))));
}

static void
test_resolve_container_box_fill(void)
{
    alia_box box = alia_resolve_container_box(
        ALIA_FILL,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f);

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f))));
}

static void
test_resolve_container_box_centered(void)
{
    alia_box box = alia_resolve_container_box(
        ALIA_CENTER,
        alia_vec2f_make(100.f, 100.f),
        0.f,
        alia_vec2f_make(10.f, 10.f),
        0.f);

    TEST_CHECK(alia_box_equal(
        box,
        alia_box_make(
            alia_vec2f_make(45.f, 45.f), alia_vec2f_make(10.f, 10.f))));
}

static void
test_resolve_alignment_flags_priority(void)
{
    // Caller absolute beats default CROSS on the cross axis.
    {
        alia_layout_flags_t const flags
            = ALIA_CENTER_X | ALIA_DEFAULT_FILL_CROSS;
        alia_layout_flags_t const resolved
            = alia_resolve_alignment_flags(flags, ALIA_MAIN_AXIS_Y);
        TEST_CHECK((resolved & ALIA_X_ALIGNMENT_MASK) == ALIA_CENTER_X);
        TEST_CHECK((resolved & ALIA_DEFAULT_ALIGNMENT_MASK) == 0);
        TEST_CHECK((resolved & ALIA_CROSS_ALIGNMENT_MASK) == 0);
    }

    // Caller CROSS beats default absolute on the cross axis.
    {
        alia_layout_flags_t const flags
            = ALIA_CENTER_CROSS | ALIA_DEFAULT_FILL_X;
        alia_layout_flags_t const resolved
            = alia_resolve_alignment_flags(flags, ALIA_MAIN_AXIS_Y);
        TEST_CHECK((resolved & ALIA_X_ALIGNMENT_MASK) == ALIA_CENTER_X);
    }

    // Default CROSS applies when nothing else claims the cross axis.
    {
        alia_layout_flags_t const flags = ALIA_DEFAULT_FILL_CROSS;
        alia_layout_flags_t const in_column
            = alia_resolve_alignment_flags(flags, ALIA_MAIN_AXIS_Y);
        TEST_CHECK((in_column & ALIA_X_ALIGNMENT_MASK) == ALIA_FILL_X);
        TEST_CHECK((in_column & ALIA_Y_ALIGNMENT_MASK) == 0);

        alia_layout_flags_t const in_row
            = alia_resolve_alignment_flags(flags, ALIA_MAIN_AXIS_X);
        TEST_CHECK((in_row & ALIA_Y_ALIGNMENT_MASK) == ALIA_FILL_Y);
        TEST_CHECK((in_row & ALIA_X_ALIGNMENT_MASK) == 0);
    }

    // Default on the main axis still applies when CROSS is set.
    {
        alia_layout_flags_t const flags
            = ALIA_CENTER_CROSS | ALIA_DEFAULT_BASELINE_Y;
        alia_layout_flags_t const in_column
            = alia_resolve_alignment_flags(flags, ALIA_MAIN_AXIS_Y);
        TEST_CHECK((in_column & ALIA_X_ALIGNMENT_MASK) == ALIA_CENTER_X);
        TEST_CHECK((in_column & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y);
    }

    // Non-alignment flags are preserved.
    {
        alia_layout_flags_t const flags = ALIA_GROW | ALIA_DEFAULT_FILL_CROSS;
        alia_layout_flags_t const resolved
            = alia_resolve_alignment_flags(flags, ALIA_MAIN_AXIS_Y);
        TEST_CHECK((resolved & ALIA_GROW) == ALIA_GROW);
        TEST_CHECK((resolved & ALIA_X_ALIGNMENT_MASK) == ALIA_FILL_X);
    }
}

void
placement_tests(void)
{
    test_resolve_growth_factor();
    test_resolve_leaf_box_fill();
    test_resolve_leaf_box_corner_alignment();
    test_resolve_leaf_box_axis_fill();
    test_resolve_leaf_box_fill_y_align_right();
    test_resolve_leaf_box_with_spacing();
    test_resolve_baseline();
    test_resolve_container_box_default_stretch();
    test_resolve_container_box_fill();
    test_resolve_container_box_centered();
    test_resolve_alignment_flags_priority();
}
