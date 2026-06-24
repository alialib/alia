#include "layout_harness.hpp"

#include <alia/abi/base/geometry/vec2.h>
#include <alia/ui/layout/components.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

TEST_CASE("layout leaf fill")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(50.f, 100.f), [&](alia_context& ctx) {
        test_leaf(ctx, alia_vec2f_make(50.f, 100.f), FILL, &box);
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(50.f, 100.f)));
}

TEST_CASE("layout leaf corner alignment")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        test_leaf(
            ctx,
            alia_vec2f_make(10.f, 10.f),
            ALIGN_RIGHT | ALIGN_BOTTOM,
            &box);
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(90.f, 90.f), alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout row with grow")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(300.f, 100.f), [&](alia_context& ctx) {
        row(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(100.f, 100.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(100.f, 0.f), alia_vec2f_make(200.f, 100.f)));
}

TEST_CASE("layout column with grow")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(100.f, 300.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(100.f, 100.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(0.f, 100.f), alia_vec2f_make(100.f, 200.f)));
}

TEST_CASE("layout grid column propagation")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(100.f, 50.f), [&](alia_context& ctx) {
        grid(ctx, [&](alia_layout_grid_handle grid_handle) {
            grid_row(ctx, grid_handle, [&]() {
                test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(30.f, 10.f), GROW);
            });
            grid_row(ctx, grid_handle, [&]() {
                test_leaf(ctx, alia_vec2f_make(10.f, 20.f), FILL, &leaf1);
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FILL, &leaf2);
            });
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 20.f), alia_vec2f_make(20.f, 20.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(20.f, 20.f), alia_vec2f_make(80.f, 20.f)));
}

TEST_CASE("layout row with gap")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(130.f, 100.f), [&](alia_context& ctx) {
        row(ctx, gap(10.f), [&]() {
            test_leaf(ctx, alia_vec2f_make(50.f, 100.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(50.f, 100.f), NO_FLAGS, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(50.f, 100.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(60.f, 0.f), alia_vec2f_make(50.f, 100.f)));
}

TEST_CASE("layout column shrink to child")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(200.f, 200.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(100.f, 100.f), ALIGN_LEFT, &box);
        });
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f)));
}

TEST_CASE("layout column grow only child")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(100.f, 200.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW, &box);
        });
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 200.f)));
}

TEST_CASE("layout grid backward propagation")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 50.f), [&](alia_context& ctx) {
        grid(ctx, [&](alia_layout_grid_handle grid_handle) {
            column(ctx, ALIGN_LEFT, [&]() {
                grid_row(ctx, grid_handle, [&]() {
                    test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                    test_spacer(ctx, alia_vec2f_make(20.f, 10.f));
                });
                test_leaf(ctx, alia_vec2f_make(10.f, 20.f), FILL, &leaf);
            });
            grid_row(ctx, grid_handle, [&]() {
                test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                test_spacer(ctx, alia_vec2f_make(50.f, 10.f));
            });
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 20.f), alia_vec2f_make(70.f, 20.f)));
}

TEST_CASE("layout grid simple two by two")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(50.f, 50.f), [&](alia_context& ctx) {
        grid(ctx, [&](alia_layout_grid_handle grid_handle) {
            grid_row(ctx, grid_handle, [&]() {
                test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                test_spacer(ctx, alia_vec2f_make(30.f, 10.f));
            });
            grid_row(ctx, grid_handle, [&]() {
                test_leaf(ctx, alia_vec2f_make(10.f, 20.f), FILL, &leaf1);
                test_leaf(
                    ctx,
                    alia_vec2f_make(10.f, 10.f),
                    ALIGN_TOP | ALIGN_LEFT,
                    &leaf2);
            });
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 20.f), alia_vec2f_make(20.f, 20.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(20.f, 20.f), alia_vec2f_make(10.f, 10.f)));
}
