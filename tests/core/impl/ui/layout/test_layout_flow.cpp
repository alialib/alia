#include "layout_harness.hpp"

#include <alia/abi/base/geometry/vec2.h>
#include <alia/ui/layout/components.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

TEST_CASE("layout horizontal flow single line")
{
    run_layout_case(
        alia_vec2f_make(50.f, 50.f),
        [&](alia_context& ctx) {
            flow(ctx, [&]() {
                test_spacer(ctx, alia_vec2f_make(10.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                test_spacer(ctx, alia_vec2f_make(30.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL);
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            flow(ctx, [&]() {
                test_spacer(ctx, alia_vec2f_make(10.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
                test_spacer(ctx, alia_vec2f_make(30.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL, &leaf2);
            });
            CHECK(check_box_eq(
                leaf1,
                alia_vec2f_make(10.f, 0.f),
                alia_vec2f_make(20.f, 10.f)));
            CHECK(check_box_eq(
                leaf2,
                alia_vec2f_make(30.f, 20.f),
                alia_vec2f_make(20.f, 10.f)));
        });
}

TEST_CASE("layout flow inside column")
{
    run_layout_case(
        alia_vec2f_make(50.f, 100.f),
        [&](alia_context& ctx) {
            column(ctx, [&]() {
                flow(ctx, [&]() {
                    test_spacer(ctx, alia_vec2f_make(10.f, 20.f));
                    test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                    test_spacer(ctx, alia_vec2f_make(30.f, 20.f));
                    test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL);
                });
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
            });
        },
        [&](alia_context& ctx) {
            alia_box flow_leaf;
            alia_box column_leaf;
            column(ctx, [&]() {
                flow(ctx, [&]() {
                    test_spacer(ctx, alia_vec2f_make(10.f, 20.f));
                    test_leaf(
                        ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &flow_leaf);
                    test_spacer(ctx, alia_vec2f_make(30.f, 20.f));
                    test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL);
                });
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &column_leaf);
            });
            CHECK(check_box_eq(
                flow_leaf,
                alia_vec2f_make(10.f, 0.f),
                alia_vec2f_make(20.f, 10.f)));
            CHECK(check_box_eq(
                column_leaf,
                alia_vec2f_make(0.f, 40.f),
                alia_vec2f_make(20.f, 10.f)));
        });
}

TEST_CASE("layout flow line wrap")
{
    run_layout_case(
        alia_vec2f_make(35.f, 50.f),
        [&](alia_context& ctx) {
            flow(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            flow(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
            });
            CHECK(check_box_eq(
                leaf1,
                alia_vec2f_make(0.f, 0.f),
                alia_vec2f_make(20.f, 10.f)));
            CHECK(check_box_eq(
                leaf2,
                alia_vec2f_make(0.f, 10.f),
                alia_vec2f_make(20.f, 10.f)));
        });
}

TEST_CASE("layout block flow vertical stack")
{
    run_layout_case(
        alia_vec2f_make(50.f, 50.f),
        [&](alia_context& ctx) {
            block_flow(ctx, [&]() {
                test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(10.f, 20.f), FILL);
                test_spacer(ctx, alia_vec2f_make(20.f, 10.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL);
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            block_flow(ctx, [&]() {
                test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(10.f, 20.f), FILL, &leaf1);
                test_spacer(ctx, alia_vec2f_make(20.f, 10.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL, &leaf2);
            });
            CHECK(check_box_eq(
                leaf1,
                alia_vec2f_make(20.f, 0.f),
                alia_vec2f_make(10.f, 20.f)));
            CHECK(check_box_eq(
                leaf2,
                alia_vec2f_make(0.f, 20.f),
                alia_vec2f_make(20.f, 10.f)));
        });
}

TEST_CASE("layout flow justify space between")
{
    run_layout_case(
        alia_vec2f_make(100.f, 20.f),
        [&](alia_context& ctx) {
            flow(ctx, JUSTIFY_SPACE_BETWEEN, [&]() {
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            flow(ctx, JUSTIFY_SPACE_BETWEEN, [&]() {
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
            });
            CHECK(check_box_eq(
                leaf1,
                alia_vec2f_make(0.f, 0.f),
                alia_vec2f_make(20.f, 10.f)));
            CHECK(check_box_eq(
                leaf2,
                alia_vec2f_make(20.f, 0.f),
                alia_vec2f_make(20.f, 10.f)));
        });
}
