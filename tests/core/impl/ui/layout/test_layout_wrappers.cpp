#include "layout_harness.hpp"

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/ui/layout/components.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

TEST_CASE("layout alignment override")
{
    run_layout_case(
        alia_vec2f_make(100.f, 100.f),
        [&](alia_context& ctx) {
            alignment_override(ctx, ALIGN_LEFT, [&]() {
                test_leaf(
                    ctx, alia_vec2f_make(10.f, 10.f), ALIGN_RIGHT | ALIGN_BOTTOM);
            });
        },
        [&](alia_context& ctx) {
            alia_box box;
            alignment_override(ctx, ALIGN_LEFT, [&]() {
                test_leaf(
                    ctx,
                    alia_vec2f_make(10.f, 10.f),
                    ALIGN_RIGHT | ALIGN_BOTTOM,
                    &box);
            });
            CHECK(check_box_eq(
                box,
                alia_vec2f_make(0.f, 90.f),
                alia_vec2f_make(10.f, 10.f)));
        });
}

TEST_CASE("layout edge offsets")
{
    run_layout_case(
        alia_vec2f_make(100.f, 100.f),
        [&](alia_context& ctx) {
            edge_offsets(
                ctx,
                alia_edge_offsets_make_trbl(10.f, 10.f, 10.f, 10.f),
                [&]() { test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL); });
        },
        [&](alia_context& ctx) {
            alia_box box;
            edge_offsets(
                ctx,
                alia_edge_offsets_make_trbl(10.f, 10.f, 10.f, 10.f),
                [&]() {
                    test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL, &box);
                });
            CHECK(check_box_eq(
                box,
                alia_vec2f_make(10.f, 10.f),
                alia_vec2f_make(80.f, 80.f)));
        });
}

TEST_CASE("layout min size constraint")
{
    run_layout_case(
        alia_vec2f_make(150.f, 150.f),
        [&](alia_context& ctx) {
            min_size_constraint(
                ctx, alia_vec2f_make(150.f, 150.f), [&]() {
                    test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL);
                });
        },
        [&](alia_context& ctx) {
            alia_box box;
            min_size_constraint(ctx, alia_vec2f_make(150.f, 150.f), [&]() {
                test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL, &box);
            });
            CHECK(check_box_eq(
                box,
                alia_vec2f_make(0.f, 0.f),
                alia_vec2f_make(150.f, 150.f)));
        });
}

TEST_CASE("layout growth override")
{
    run_layout_case(
        alia_vec2f_make(300.f, 100.f),
        [&](alia_context& ctx) {
            row(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(100.f, 100.f));
                growth_override(ctx, 2.f, [&]() {
                    test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW);
                });
            });
        },
        [&](alia_context& ctx) {
            alia_box leaf1;
            alia_box leaf2;
            row(ctx, [&]() {
                test_leaf(ctx, alia_vec2f_make(100.f, 100.f), NO_FLAGS, &leaf1);
                growth_override(ctx, 2.f, [&]() {
                    test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW, &leaf2);
                });
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
