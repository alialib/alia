#include <alia/test/layout/layout_test_helpers.hpp>

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/ui/layout/api.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

TEST_CASE("layout alignment override")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        alignment_override(ctx, ALIGN_LEFT, [&]() {
            test_leaf(
                ctx,
                alia_vec2f_make(10.f, 10.f),
                ALIGN_RIGHT | ALIGN_BOTTOM,
                &box);
        });
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(0.f, 90.f), alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout edge offsets")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        edge_offsets(
            ctx, alia_edge_offsets_make_trbl(10.f, 10.f, 10.f, 10.f), [&]() {
                test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL, &box);
            });
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(10.f, 10.f), alia_vec2f_make(80.f, 80.f)));
}

TEST_CASE("layout min size constraint")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(150.f, 150.f), [&](alia_context& ctx) {
        min_size_constraint(ctx, alia_vec2f_make(150.f, 150.f), [&]() {
            test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL, &box);
        });
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(150.f, 150.f)));
}

TEST_CASE("layout growth override")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(300.f, 100.f), [&](alia_context& ctx) {
        row(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(100.f, 100.f), NO_FLAGS, &leaf1);
            growth_override(ctx, 2.f, [&]() {
                test_leaf(ctx, alia_vec2f_make(0.f, 0.f), FILL | GROW, &leaf2);
            });
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(100.f, 0.f), alia_vec2f_make(200.f, 100.f)));
}

TEST_CASE("layout edge offsets with flags")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        edge_offsets(
            ctx,
            alia_edge_offsets_make_trbl(10.f, 10.f, 10.f, 10.f),
            ALIGN_LEFT,
            [&]() {
                test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL, &box);
            });
    });
    CHECK(check_box_eq(
        box, alia_vec2f_make(10.f, 10.f), alia_vec2f_make(80.f, 80.f)));
}

TEST_CASE("layout min size inside row")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(200.f, 50.f), [&](alia_context& ctx) {
        row(ctx, [&]() {
            min_size_constraint(ctx, alia_vec2f_make(150.f, 50.f), [&]() {
                test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL, &leaf);
            });
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(150.f, 50.f)));
}

TEST_CASE("layout spacer applies theme scale")
{
    alia_box leaf;
    run_layout_case_with_scale(
        2.f, alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
            column(ctx, [&]() {
                spacer(ctx, alia_vec2f_make(10.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), NO_FLAGS, &leaf);
            });
        });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 40.f), alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout spacer is flush against style spacing")
{
    alia_box leaf;
    run_layout_case_with_spacing(
        8.f, alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
            column(ctx, [&]() {
                spacer(ctx, alia_vec2f_make(10.f, 20.f));
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &leaf);
            });
        });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 20.f), alia_vec2f_make(10.f, 10.f)));
}
