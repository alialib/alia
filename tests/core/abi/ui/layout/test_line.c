#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/utilities/line.h>

#define TEST_NO_MAIN
#include "acutest.h"

static void
test_justify_line_start(void)
{
    alia_layout_line_justification_spacing spacing
        = alia_layout_justify_line(ALIA_JUSTIFY_START, 100.f, 3);
    TEST_CHECK(spacing.before_items == 0.f);
    TEST_CHECK(spacing.between_items == 0.f);
}

static void
test_justify_line_end(void)
{
    alia_layout_line_justification_spacing spacing
        = alia_layout_justify_line(ALIA_JUSTIFY_END, 100.f, 3);
    TEST_CHECK(spacing.before_items == 100.f);
    TEST_CHECK(spacing.between_items == 0.f);
}

static void
test_justify_line_center(void)
{
    alia_layout_line_justification_spacing spacing
        = alia_layout_justify_line(ALIA_JUSTIFY_CENTER, 100.f, 3);
    TEST_CHECK(spacing.before_items == 50.f);
    TEST_CHECK(spacing.between_items == 0.f);
}

static void
test_justify_line_space_between(void)
{
    alia_layout_line_justification_spacing spacing
        = alia_layout_justify_line(ALIA_JUSTIFY_SPACE_BETWEEN, 100.f, 3);
    TEST_CHECK(spacing.before_items == 0.f);
    TEST_CHECK(spacing.between_items == 50.f);
}

static void
test_justify_line_space_around(void)
{
    alia_layout_line_justification_spacing spacing
        = alia_layout_justify_line(ALIA_JUSTIFY_SPACE_AROUND, 120.f, 3);
    TEST_CHECK(spacing.before_items == 20.f);
    TEST_CHECK(spacing.between_items == 40.f);
}

static void
test_justify_line_space_evenly(void)
{
    alia_layout_line_justification_spacing spacing
        = alia_layout_justify_line(ALIA_JUSTIFY_SPACE_EVENLY, 100.f, 3);
    TEST_CHECK(spacing.before_items == 25.f);
    TEST_CHECK(spacing.between_items == 25.f);
}

static void
test_line_final_height(void)
{
    alia_line_requirements line = {.height = 10.f, .ascent = 8.f, .descent = 5.f};
    alia_layout_line_finalize_height(&line);
    TEST_CHECK(line.height == 13.f);
}

static void
test_line_finalize_height_with_minimum(void)
{
    alia_line_requirements line = {.height = 10.f, .ascent = 4.f, .descent = 2.f};
    alia_layout_line_finalize_height_with_minimum(&line, 20.f);
    TEST_CHECK(line.height == 20.f);
}

static void
test_justify_flags_for_incomplete_line(void)
{
    alia_layout_flags_t flags = alia_layout_justify_flags_for_incomplete_line(
        ALIA_JUSTIFY_SPACE_BETWEEN);
    TEST_CHECK(flags == ALIA_JUSTIFY_START);
}

void
line_tests(void)
{
    test_justify_line_start();
    test_justify_line_end();
    test_justify_line_center();
    test_justify_line_space_between();
    test_justify_line_space_around();
    test_justify_line_space_evenly();
    test_line_final_height();
    test_line_finalize_height_with_minimum();
    test_justify_flags_for_incomplete_line();
}
