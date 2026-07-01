#include <alia/test/layout/layout_test_helpers.hpp>

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/impl/events.hpp>
#include <alia/ui/layout/api.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

TEST_CASE("layout row provide box")
{
    alia_box container;
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 50.f), [&](alia_context& ctx) {
        row(ctx, &container, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf);
        });
    });
    CHECK(check_box_eq(
        container, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 50.f)));
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout column provide box")
{
    alia_box container;
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 80.f), [&](alia_context& ctx) {
        column(ctx, &container, [&]() {
            test_leaf(ctx, alia_vec2f_make(40.f, 20.f), NO_FLAGS, &leaf);
        });
    });
    CHECK(check_box_eq(
        container, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 80.f)));
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(40.f, 20.f)));
}

TEST_CASE("layout flow provide box")
{
    alia_box container;
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, &container, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf);
        });
    });
    CHECK(check_box_eq(
        container, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 30.f)));
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout edge offsets provide box")
{
    alia_box outer;
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        if (is_refresh_event(ctx))
        {
            alia_layout_edge_offsets_begin(
                &ctx,
                alia_edge_offsets_make_trbl(10.f, 10.f, 10.f, 10.f),
                raw_code(PROVIDE_BOX));
            test_leaf(ctx, alia_vec2f_make(50.f, 50.f));
            alia_layout_edge_offsets_end(&ctx);
        }
        else
        {
            alia_layout_box_array const boxes
                = alia_layout_consume_box_array(&ctx);
            CHECK(boxes.count == 1);
            outer = boxes.boxes[0];
            test_leaf(ctx, alia_vec2f_make(50.f, 50.f), FILL, &leaf);
        }
    });
    CHECK(check_box_eq(
        outer, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(100.f, 100.f)));
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(10.f, 10.f), alia_vec2f_make(50.f, 50.f)));
}
