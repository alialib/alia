#include "layout_harness.hpp"

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/impl/events.hpp>
#include <alia/ui/layout/api.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::layout_test;

TEST_CASE("layout horizontal flow single line")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(50.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_spacer(ctx, alia_vec2f_make(10.f, 20.f));
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_spacer(ctx, alia_vec2f_make(30.f, 20.f));
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(10.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(30.f, 20.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow inside column")
{
    alia_box flow_leaf;
    alia_box column_leaf;
    run_layout_case(alia_vec2f_make(50.f, 100.f), [&](alia_context& ctx) {
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
    });
    CHECK(check_box_eq(
        flow_leaf, alia_vec2f_make(10.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        column_leaf, alia_vec2f_make(0.f, 40.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow line wrap")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(35.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(0.f, 10.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout block flow vertical stack")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(50.f, 50.f), [&](alia_context& ctx) {
        block_flow(ctx, [&]() {
            test_spacer(ctx, alia_vec2f_make(20.f, 20.f));
            test_leaf(ctx, alia_vec2f_make(10.f, 20.f), FILL, &leaf1);
            test_spacer(ctx, alia_vec2f_make(20.f, 10.f));
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FILL, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(20.f, 0.f), alia_vec2f_make(10.f, 20.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(0.f, 20.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow justify space between single line")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(100.f, 20.f), [&](alia_context& ctx) {
        flow(ctx, JUSTIFY_SPACE_BETWEEN, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_flow_spring(ctx);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(20.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow justify space between full line")
{
    alia_box leaf1;
    alia_box leaf2;
    alia_box leaf3;
    alia_box leaf4;
    run_layout_case(alia_vec2f_make(70.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, JUSTIFY_SPACE_BETWEEN, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_flow_spring(ctx);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
            test_flow_spring(ctx);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf3);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf4);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(25.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf3, alia_vec2f_make(50.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf4, alia_vec2f_make(0.f, 10.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow justify space around full line")
{
    alia_box leaf1;
    alia_box leaf2;
    alia_box leaf3;
    alia_box leaf4;
    run_layout_case(alia_vec2f_make(70.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, JUSTIFY_SPACE_AROUND, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_flow_spring(ctx);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
            test_flow_spring(ctx);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf3);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf4);
        });
    });
    CHECK(check_box_near(
        leaf1, alia_vec2f_make(1.6666667f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_near(
        leaf2,
        alia_vec2f_make(25.f, 0.f),
        alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_near(
        leaf3,
        alia_vec2f_make(48.333332f, 0.f),
        alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf4, alia_vec2f_make(0.f, 10.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow justify space evenly full line")
{
    alia_box leaf1;
    alia_box leaf2;
    alia_box leaf3;
    alia_box leaf4;
    run_layout_case(alia_vec2f_make(70.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, JUSTIFY_SPACE_EVENLY, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_flow_spring(ctx);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
            test_flow_spring(ctx);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf3);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf4);
        });
    });
    CHECK(check_box_near(
        leaf1, alia_vec2f_make(2.5f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_near(
        leaf2, alia_vec2f_make(25.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_near(
        leaf3, alia_vec2f_make(47.5f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf4, alia_vec2f_make(0.f, 10.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow justify end")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(100.f, 20.f), [&](alia_context& ctx) {
        flow(ctx, JUSTIFY_END, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(60.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(80.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow justify center")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(100.f, 20.f), [&](alia_context& ctx) {
        flow(ctx, JUSTIFY_CENTER, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(30.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(50.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow line gap")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(35.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, line_gap(10.f), [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(0.f, 20.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow minimum line height")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(50.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, minimum_line_height(30.f), [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf);
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout block flow justify center")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(50.f, 100.f), [&](alia_context& ctx) {
        block_flow(ctx, JUSTIFY_CENTER, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(5.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(25.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow incomplete line justify")
{
    alia_box leaf1;
    alia_box leaf2;
    alia_box leaf3;
    run_layout_case(alia_vec2f_make(45.f, 40.f), [&](alia_context& ctx) {
        flow(ctx, FILL | JUSTIFY_SPACE_BETWEEN, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf1);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf2);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf3);
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(20.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf3, alia_vec2f_make(0.f, 10.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow growth override passthrough")
{
    alia_box plain_a;
    alia_box plain_b;
    alia_box plain_c;
    run_layout_case(alia_vec2f_make(100.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(10.f, 10.f), NO_FLAGS, &plain_a);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &plain_b);
            test_leaf(ctx, alia_vec2f_make(15.f, 10.f), NO_FLAGS, &plain_c);
        });
    });

    alia_box wrapped_a;
    alia_box wrapped_b;
    alia_box wrapped_c;
    run_layout_case(alia_vec2f_make(100.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(10.f, 10.f), NO_FLAGS, &wrapped_a);
            growth_override(ctx, 2.f, [&]() {
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &wrapped_b);
            });
            test_leaf(ctx, alia_vec2f_make(15.f, 10.f), NO_FLAGS, &wrapped_c);
        });
    });

    CHECK(check_box_eq(wrapped_a, plain_a.min, plain_a.size));
    CHECK(check_box_eq(wrapped_b, plain_b.min, plain_b.size));
    CHECK(check_box_eq(wrapped_c, plain_c.min, plain_c.size));
}

TEST_CASE("layout flow edge offsets single leaf")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            edge_offsets(
                ctx,
                alia_edge_offsets_make_trbl(2.f, 5.f, 2.f, 5.f),
                [&]() {
                    test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf);
                });
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(5.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow edge offsets between plain leaves")
{
    alia_box leaf_a;
    alia_box leaf_b;
    alia_box leaf_c;
    run_layout_case(alia_vec2f_make(100.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(10.f, 10.f), NO_FLAGS, &leaf_a);
            edge_offsets(
                ctx,
                alia_edge_offsets_make_trbl(0.f, 5.f, 0.f, 5.f),
                [&]() {
                    test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf_b);
                });
            test_leaf(ctx, alia_vec2f_make(15.f, 10.f), NO_FLAGS, &leaf_c);
        });
    });
    CHECK(check_box_eq(
        leaf_a, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(10.f, 10.f)));
    CHECK(check_box_eq(
        leaf_b, alia_vec2f_make(15.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        leaf_c, alia_vec2f_make(40.f, 0.f), alia_vec2f_make(15.f, 10.f)));
}

TEST_CASE("layout flow edge offsets line wrap")
{
    alia_box leaf_a;
    alia_box leaf_b;
    run_layout_case(alia_vec2f_make(35.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(10.f, 10.f), NO_FLAGS, &leaf_a);
            edge_offsets(
                ctx,
                alia_edge_offsets_make_trbl(0.f, 8.f, 0.f, 8.f),
                [&]() {
                    test_leaf(ctx, alia_vec2f_make(15.f, 10.f), NO_FLAGS, &leaf_b);
                });
        });
    });
    CHECK(check_box_eq(
        leaf_a, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(10.f, 10.f)));
    CHECK(check_box_eq(
        leaf_b, alia_vec2f_make(8.f, 10.f), alia_vec2f_make(15.f, 10.f)));
}

TEST_CASE("layout flow edge offsets groups multiple children")
{
    alia_box leaf1;
    alia_box leaf2;
    run_layout_case(alia_vec2f_make(100.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            edge_offsets(
                ctx,
                alia_edge_offsets_make_trbl(0.f, 5.f, 0.f, 5.f),
                [&]() {
                    test_leaf(ctx, alia_vec2f_make(10.f, 10.f), NO_FLAGS, &leaf1);
                    test_leaf(ctx, alia_vec2f_make(15.f, 10.f), NO_FLAGS, &leaf2);
                });
        });
    });
    CHECK(check_box_eq(
        leaf1, alia_vec2f_make(5.f, 0.f), alia_vec2f_make(10.f, 10.f)));
    CHECK(check_box_eq(
        leaf2, alia_vec2f_make(15.f, 0.f), alia_vec2f_make(15.f, 10.f)));
}

TEST_CASE("layout flow edge offsets provide box single line")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 30.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(10.f, 10.f));
            if (is_refresh_event(ctx))
            {
                alia_layout_edge_offsets_begin(
                    &ctx,
                    alia_edge_offsets_make_trbl(0.f, 5.f, 0.f, 5.f),
                    raw_code(PROVIDE_BOX));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                alia_layout_edge_offsets_end(&ctx);
            }
            else
            {
                alia_layout_box_array const boxes
                    = alia_layout_consume_box_array(&ctx);
                CHECK(boxes.count == 1);
                CHECK(check_box_eq(
                    boxes.boxes[0],
                    alia_vec2f_make(10.f, 5.f),
                    alia_vec2f_make(30.f, 10.f)));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f), NO_FLAGS, &leaf);
            }
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(15.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout flow edge offsets provide box wrapped lines")
{
    run_layout_case(alia_vec2f_make(40.f, 50.f), [&](alia_context& ctx) {
        flow(ctx, [&]() {
            if (is_refresh_event(ctx))
            {
                alia_layout_edge_offsets_begin(
                    &ctx,
                    alia_edge_offsets_make_trbl(0.f, 4.f, 0.f, 4.f),
                    raw_code(PROVIDE_BOX));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                alia_layout_edge_offsets_end(&ctx);
            }
            else
            {
                alia_layout_box_array const boxes
                    = alia_layout_consume_box_array(&ctx);
                CHECK(boxes.count == 2);
                CHECK(check_box_eq(
                    boxes.boxes[0],
                    alia_vec2f_make(0.f, 5.f),
                    alia_vec2f_make(28.f, 10.f)));
                CHECK(check_box_eq(
                    boxes.boxes[1],
                    alia_vec2f_make(0.f, 15.f),
                    alia_vec2f_make(28.f, 10.f)));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
                test_leaf(ctx, alia_vec2f_make(20.f, 10.f));
            }
        });
    });
}
