#include "layout_harness.hpp"

#include <alia/abi/base/geometry/vec2.h>
#include <alia/ui/layout/components.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

namespace {

template<class Content, class Check>
void
run_layout_case(alia_vec2f assigned, Content&& content, Check&& check)
{
    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    layout_test_fixture_run_refresh(
        fixture, [&](alia_context* ctx) { content(*ctx); });
    layout_test_fixture_resolve(fixture, assigned);
    layout_test_fixture_run_spatial(
        fixture, [&](alia_context* ctx) { check(*ctx); });

    layout_test_fixture_destroy(fixture);
}

} // namespace

TEST_CASE("layout leaf fill")
{
    run_layout_case(
        alia_vec2f_make(50.f, 100.f),
        [&](alia_context& ctx) { test_leaf(ctx, alia_vec2f_make(50.f, 100.f), FILL); },
        [&](alia_context& ctx) {
            alia_box box;
            test_leaf(ctx, alia_vec2f_make(50.f, 100.f), FILL, &box);
            CHECK(check_box_eq(
                box,
                alia_vec2f_make(0.f, 0.f),
                alia_vec2f_make(50.f, 100.f)));
        });
}

TEST_CASE("layout leaf corner alignment")
{
    run_layout_case(
        alia_vec2f_make(100.f, 100.f),
        [&](alia_context& ctx) {
            test_leaf(
                ctx, alia_vec2f_make(10.f, 10.f), ALIGN_RIGHT | ALIGN_BOTTOM);
        },
        [&](alia_context& ctx) {
            alia_box box;
            test_leaf(
                ctx,
                alia_vec2f_make(10.f, 10.f),
                ALIGN_RIGHT | ALIGN_BOTTOM,
                &box);
            CHECK(check_box_eq(
                box,
                alia_vec2f_make(90.f, 90.f),
                alia_vec2f_make(10.f, 10.f)));
        });
}

TEST_CASE("layout row with grow")
{
    run_layout_case(
        alia_vec2f_make(300.f, 100.f),
        [&](alia_context& ctx) {
            row(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(100.f, 100.f));
                test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW);
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            row(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(100.f, 100.f), NO_FLAGS, &leaf1);
                test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW, &leaf2);
            });
            CHECK(check_box_eq(
                leaf1,
                alia_vec2f_make(0.f, 0.f),
                alia_vec2f_make(100.f, 100.f)));
            CHECK(check_box_eq(
                leaf2,
                alia_vec2f_make(100.f, 0.f),
                alia_vec2f_make(200.f, 100.f)));
        });
}

TEST_CASE("layout column with grow")
{
    run_layout_case(
        alia_vec2f_make(100.f, 300.f),
        [&](alia_context& ctx) {
            column(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(100.f, 100.f));
                test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW);
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            column(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(100.f, 100.f), NO_FLAGS, &leaf1);
                test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW, &leaf2);
            });
            CHECK(check_box_eq(
                leaf1,
                alia_vec2f_make(0.f, 0.f),
                alia_vec2f_make(100.f, 100.f)));
            CHECK(check_box_eq(
                leaf2,
                alia_vec2f_make(0.f, 100.f),
                alia_vec2f_make(100.f, 200.f)));
        });
}

TEST_CASE("layout grid column propagation")
{
    run_layout_case(
        alia_vec2f_make(100.f, 50.f),
        [&](alia_context& ctx) {
            grid(ctx, [&](alia_layout_grid_handle grid_handle) {
                grid_row(ctx, grid_handle, [&]() {
                    test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                    test_leaf(ctx, alia_vec2f_make(30.f, 10.f), GROW);
                });
                grid_row(ctx, grid_handle, [&]() {
                    test_leaf(
                        ctx, alia_vec2f_make(10.f, 20.f), FILL);
                    test_leaf(
                        ctx, alia_vec2f_make(10.f, 10.f), FILL);
                });
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            grid(ctx, [&](alia_layout_grid_handle grid_handle) {
                grid_row(ctx, grid_handle, [&]() {
                    test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
                    test_leaf(ctx, alia_vec2f_make(30.f, 10.f), GROW);
                });
                grid_row(ctx, grid_handle, [&]() {
                    test_leaf(
                        ctx, alia_vec2f_make(10.f, 20.f), FILL, &leaf1);
                    test_leaf(
                        ctx, alia_vec2f_make(10.f, 10.f), FILL, &leaf2);
                });
            });
            CHECK(check_box_eq(
                leaf1,
                alia_vec2f_make(0.f, 20.f),
                alia_vec2f_make(20.f, 20.f)));
            CHECK(check_box_eq(
                leaf2,
                alia_vec2f_make(20.f, 20.f),
                alia_vec2f_make(80.f, 20.f)));
        });
}
