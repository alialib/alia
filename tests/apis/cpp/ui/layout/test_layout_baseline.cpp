#include <alia/test/layout/layout_test_helpers.hpp>

#include <alia/abi/base/geometry/vec2.h>
#include <alia/ui/layout/api.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

TEST_CASE("layout row shared baseline y")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(60.f, 50.f), [&](alia_context& ctx) {
        row(ctx, [&]() {
            test_leaf(
                ctx,
                alia_vec2f_make(20.f, 10.f),
                BASELINE_Y,
                &leaf1,
                8.f,
                2.f);
            test_leaf(
                ctx,
                alia_vec2f_make(20.f, 12.f),
                BASELINE_Y,
                &leaf2,
                4.f,
                8.f);
        });
    });
    // Shared row baseline at y=25; tops differ by ascent (25-8=17, 25-4=21).
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 17.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(20.f, 21.f), alia_vec2f_make(20.f, 12.f)));
}

TEST_CASE("layout row baseline group align top")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(60.f, 50.f), [&](alia_context& ctx) {
        row(ctx, BASELINE_GROUP_ALIGN_TOP, [&]() {
            test_leaf(
                ctx,
                alia_vec2f_make(20.f, 10.f),
                BASELINE_Y,
                &leaf1,
                8.f,
                2.f);
            test_leaf(
                ctx,
                alia_vec2f_make(20.f, 12.f),
                BASELINE_Y,
                &leaf2,
                4.f,
                8.f);
        });
    });
    // Group pinned to top; shared baseline at y=8 within the row.
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(20.f, 4.f), alia_vec2f_make(20.f, 12.f)));
}

TEST_CASE("layout column baseline cross stacks children")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(50.f, 60.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            test_leaf(
                ctx,
                alia_vec2f_make(40.f, 10.f),
                BASELINE_CROSS,
                &leaf1,
                8.f,
                2.f);
            test_leaf(
                ctx,
                alia_vec2f_make(40.f, 12.f),
                BASELINE_CROSS,
                &leaf2,
                4.f,
                8.f);
        });
    });
    // BASELINE_CROSS is the cross-axis baseline flag for column children (X is
    // cross).
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(50.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(0.f, 10.f), alia_vec2f_make(50.f, 12.f)));
    CHECK(leaf2.min.y == leaf1.min.y + leaf1.size.y);
}

TEST_CASE("layout flow line height from tallest fragment")
{
    alia_box short_leaf;
    alia_box tall_leaf;
    run_layout_case(alia_vec2f_make(50.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_leaf(
                ctx,
                alia_vec2f_make(20.f, 8.f),
                NO_FLAGS,
                &short_leaf,
                6.f,
                2.f);
            test_leaf(
                ctx,
                alia_vec2f_make(20.f, 16.f),
                NO_FLAGS,
                &tall_leaf,
                12.f,
                4.f);
        });
    });
    CHECK(check_box_eq(
        short_leaf, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 8.f)));
    CHECK(check_box_eq(
        tall_leaf, alia_vec2f_make(20.f, 0.f), alia_vec2f_make(20.f, 16.f)));
}
