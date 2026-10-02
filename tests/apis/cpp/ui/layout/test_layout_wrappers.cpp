#include <alia/test/layout/layout_test_helpers.hpp>

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/library.h>
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

TEST_CASE("layout min_axis_size length follows parent main axis")
{
    alia_box after_row;
    alia_box after_column;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            row(ctx, [&]() {
                min_axis_size_constraint(
                    ctx, alia_vec2f_make(20.f, 0.f), [&]() {
                        test_leaf(ctx, alia_vec2f_make(5.f, 5.f), FLUSH);
                    });
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &after_row);
            });
            column(ctx, [&]() {
                min_axis_size_constraint(
                    ctx, alia_vec2f_make(20.f, 0.f), [&]() {
                        test_leaf(ctx, alia_vec2f_make(5.f, 5.f), FLUSH);
                    });
                test_leaf(
                    ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &after_column);
            });
        });
    });
    // In a row, length floors x, so the following leaf starts at x = 20.
    CHECK(check_box_eq(
        after_row, alia_vec2f_make(20.f, 0.f), alia_vec2f_make(10.f, 10.f)));
    // In a column, length floors y. Nested column sits below the prior row
    // (height 10), so the following leaf is at y = 10 + 20 = 30.
    CHECK(check_box_eq(
        after_column,
        alia_vec2f_make(0.f, 30.f),
        alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout apply_layout length opens min_axis_size")
{
    alia_box after;
    run_layout_case(alia_vec2f_make(100.f, 50.f), [&](alia_context& ctx) {
        row(ctx, [&]() {
            // `length` on a container goes through apply_layout →
            // min_axis_size.
            column(ctx, length(30.f), [&]() {
                test_leaf(ctx, alia_vec2f_make(5.f, 5.f), FLUSH);
            });
            test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &after);
        });
    });
    CHECK(check_box_eq(
        after, alia_vec2f_make(30.f, 0.f), alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout separator content size is axis-relative")
{
    alia_box after_row;
    alia_box after_column;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            row(ctx, [&]() {
                // (length, breadth) = (20, 4) inside a row: 20 wide by 4 tall.
                if (is_refresh_event(ctx))
                {
                    alia_separator(
                        &ctx,
                        raw_code(AXIS_RELATIVE_SIZE),
                        alia_vec2f_make(20.f, 4.f));
                }
                else
                {
                    (void) alia_layout_consume_box(&ctx);
                }
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &after_row);
            });
            column(ctx, [&]() {
                // (length, breadth) = (20, 4) inside a column: 4 wide by 20
                // tall.
                if (is_refresh_event(ctx))
                {
                    alia_separator(
                        &ctx,
                        raw_code(AXIS_RELATIVE_SIZE),
                        alia_vec2f_make(20.f, 4.f));
                }
                else
                {
                    (void) alia_layout_consume_box(&ctx);
                }
                test_leaf(
                    ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &after_column);
            });
        });
    });
    CHECK(check_box_eq(
        after_row, alia_vec2f_make(20.f, 0.f), alia_vec2f_make(10.f, 10.f)));
    // Prior row is 10 tall; separator adds 20 along the column main axis.
    CHECK(check_box_eq(
        after_column,
        alia_vec2f_make(0.f, 30.f),
        alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout clamped centers by default")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(200.f, 100.f), [&](alia_context& ctx) {
        clamped(ctx, alia_vec2f_make(80.f, 40.f), [&]() {
            test_leaf(ctx, alia_vec2f_make(80.f, 40.f), FILL, &leaf);
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(60.f, 30.f), alia_vec2f_make(80.f, 40.f)));
}

TEST_CASE("layout clamped respects alignment")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(200.f, 100.f), [&](alia_context& ctx) {
        clamped(
            ctx, alia_vec2f_make(80.f, 40.f), ALIGN_LEFT | ALIGN_TOP, [&]() {
                test_leaf(ctx, alia_vec2f_make(80.f, 40.f), FILL, &leaf);
            });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(80.f, 40.f)));
}

TEST_CASE("layout clamped width-only leaves height uncapped")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(200.f, 100.f), [&](alia_context& ctx) {
        clamped(ctx, 80.f, [&]() {
            test_leaf(ctx, alia_vec2f_make(80.f, 100.f), FILL, &leaf);
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(60.f, 0.f), alia_vec2f_make(80.f, 100.f)));
}

TEST_CASE("layout clamped yields to child min size")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(200.f, 100.f), [&](alia_context& ctx) {
        clamped(ctx, alia_vec2f_make(80.f, 40.f), [&]() {
            test_leaf(ctx, alia_vec2f_make(100.f, 50.f), FILL, &leaf);
        });
    });
    // Child min exceeds the cap, so the content region grows to fit.
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(50.f, 25.f), alia_vec2f_make(100.f, 50.f)));
}

TEST_CASE("layout spacer applies theme scale")
{
    alia_box leaf;
    run_layout_case_with_scale(
        2.f, alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
            column(ctx, [&]() {
                spacer(ctx, width(10.f) | height(20.f));
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), NO_FLAGS, &leaf);
            });
        });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 40.f), alia_vec2f_make(20.f, 20.f)));
}

TEST_CASE("layout spacer is flush against style spacing")
{
    alia_box leaf;
    run_layout_case_with_spacing(
        8.f, alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
            column(ctx, [&]() {
                spacer(ctx, width(10.f) | height(20.f));
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &leaf);
            });
        });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 20.f), alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout spacer width and height layout pieces")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            spacer(ctx, height(30.f));
            test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &leaf);
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 30.f), alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout spacer SPACED opts out of default flush")
{
    alia_box leaf;
    run_layout_case_with_spacing(
        8.f, alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
            column(ctx, [&]() {
                spacer(ctx, height(20.f) | SPACED);
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &leaf);
            });
        });
    // spacer content 20 + theme spacing 8 on each side
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(0.f, 36.f), alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout spacer grow with zero size")
{
    alia_box leaf;
    run_layout_case(alia_vec2f_make(100.f, 50.f), [&](alia_context& ctx) {
        row(ctx, [&]() {
            spacer(ctx, GROW);
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), FLUSH, &leaf);
        });
    });
    CHECK(check_box_eq(
        leaf, alia_vec2f_make(80.f, 0.f), alia_vec2f_make(20.f, 10.f)));
}

TEST_CASE("layout spacer length follows parent main axis")
{
    alia_box after_row;
    alia_box after_column;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            row(ctx, [&]() {
                spacer(ctx, length(20.f));
                test_leaf(ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &after_row);
            });
            column(ctx, [&]() {
                spacer(ctx, length(20.f));
                test_leaf(
                    ctx, alia_vec2f_make(10.f, 10.f), FLUSH, &after_column);
            });
        });
    });
    CHECK(check_box_eq(
        after_row, alia_vec2f_make(20.f, 0.f), alia_vec2f_make(10.f, 10.f)));
    CHECK(check_box_eq(
        after_column,
        alia_vec2f_make(0.f, 30.f),
        alia_vec2f_make(10.f, 10.f)));
}

TEST_CASE("layout spacer breadth follows parent cross axis")
{
    alia_box tall;
    alia_box wide;
    run_layout_case(alia_vec2f_make(100.f, 100.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            row(ctx, [&]() {
                spacer(ctx, length(10.f) | breadth(30.f));
                test_leaf(ctx, alia_vec2f_make(5.f, 5.f), FLUSH, &tall);
            });
            column(ctx, [&]() {
                spacer(ctx, length(10.f) | breadth(30.f));
                test_leaf(ctx, alia_vec2f_make(5.f, 5.f), FLUSH, &wide);
            });
        });
    });
    // In a row, `length` is x (10) and `breadth` is y (30), so the following
    // leaf is at x = 10 and the row is 30 tall. (The leaf stays top-aligned at
    // y = 0.)
    CHECK(check_box_eq(
        tall, alia_vec2f_make(10.f, 0.f), alia_vec2f_make(5.f, 5.f)));
    // In a column, `length` is y (10) and `breadth` is x (30), so the
    // following leaf is at y = 10 within this nested column. (`y = 40` is
    // absolute; prior row height is 30.)
    CHECK(check_box_eq(
        wide, alia_vec2f_make(0.f, 40.f), alia_vec2f_make(5.f, 5.f)));
}

TEST_CASE("layout leaf AXIS_RELATIVE_SIZE remaps under column")
{
    alia_box box;
    run_layout_case(alia_vec2f_make(50.f, 50.f), [&](alia_context& ctx) {
        column(ctx, [&]() {
            test_leaf(
                ctx,
                alia_vec2f_make(20.f, 8.f),
                FLUSH | AXIS_RELATIVE_SIZE,
                &box);
        });
    });
    // (`length` = 20, `breadth` = 8) in a column equates to a size of (8, 20).
    CHECK(check_box_eq(
        box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(8.f, 20.f)));
}
