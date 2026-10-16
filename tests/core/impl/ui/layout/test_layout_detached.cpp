#include <alia/test/layout/layout_fixture.hpp>

#include <alia/abi/base/geometry/box.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/layout/detached.h>
#include <alia/abi/ui/layout/placement.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/layout/utilities/defaults.h>
#include <alia/abi/ui/layout/utilities/emission.h>
#include <alia/impl/base/arena.hpp>

#include <doctest/doctest.h>

using namespace alia;

namespace {

// Stands in for a scroll view: in-flow measure reports a small minimum, while
// `measure_preferred_size` reports the content size. Assign of a shorter box
// sets `overflow`.
struct shrinkable_node
{
    alia_layout_node base;
    bool* overflow;
};

float const shrinkable_content_height = 80.f;
float const shrinkable_minimum_height = 20.f;

alia_horizontal_requirements
shrinkable_measure_horizontal(
    alia_measurement_context*, alia_main_axis_index, alia_layout_node*)
{
    return {.min_size = 40.f, .growth_factor = 0.f};
}

alia_vertical_requirements
shrinkable_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index,
    alia_layout_node*,
    float)
{
    float const height = ctx->measure_preferred_size
                           ? shrinkable_content_height
                           : shrinkable_minimum_height;
    return {
        .min_size = height,
        .growth_factor = 0.f,
        .ascent = height,
        .descent = 0.f};
}

void
shrinkable_assign_boxes(
    alia_placement_context*,
    alia_main_axis_index,
    alia_layout_node* node,
    alia_box box,
    float)
{
    auto& n = *reinterpret_cast<shrinkable_node*>(node);
    if (n.overflow)
        *n.overflow = box.size.y < shrinkable_content_height;
}

alia_layout_node_vtable shrinkable_vtable
    = {shrinkable_measure_horizontal,
       shrinkable_measure_vertical,
       shrinkable_assign_boxes,
       alia_default_count_flow_emissions,
       alia_default_emit_flow_fragments,
       alia_default_read_fragment_placements};

void
emit_shrinkable(alia_context* ctx, bool* overflow)
{
    auto& emission = ctx->layout->emission;
    auto* node = arena_alloc<shrinkable_node>(emission.arena);
    *emission.next_ptr = &node->base;
    emission.next_ptr = &node->base.next_sibling;
    *node = shrinkable_node{
        .base = {.vtable = &shrinkable_vtable, .next_sibling = nullptr},
        .overflow = overflow};
}

bool
check_box_eq(alia_box box, alia_vec2f min, alia_vec2f size)
{
    return alia_box_equal(box, alia_box_make(min, size));
}

void
test_leaf(
    alia_context* ctx,
    alia_vec2f size,
    alia_layout_flags_t flags,
    alia_box* out)
{
    if (alia_layout_context_is_refresh(ctx))
    {
        alia_layout_leaf_emit(
            ctx,
            alia_layout_content_metrics{
                .size = size, .ascent = 0.f, .descent = 0.f},
            flags);
    }
    else
    {
        alia_box box = alia_layout_consume_box(ctx);
        if (out)
            *out = box;
    }
}

template<class Fn>
void
row(alia_context* ctx, Fn&& fn)
{
    alia_layout_row_begin(ctx, 0, 0.f);
    fn();
    alia_layout_row_end(ctx);
}

template<class Fn>
void
column(alia_context* ctx, Fn&& fn)
{
    alia_layout_column_begin(ctx, 0, 0.f);
    fn();
    alia_layout_column_end(ctx);
}

} // namespace

TEST_CASE("detached layout assigns separately from the main tree")
{
    alia_box main_box{};
    alia_box detached_box{};
    alia_layout_detached data{};
    alia_vec2f preferred{};

    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [&](alia_context* ctx) {
        row(ctx, [&]() {
            if (!alia_layout_context_is_refresh(ctx))
            {
                preferred = alia_layout_detached_measure(ctx, &data);
                alia_layout_detached_assign(ctx, &data, preferred);
            }
            alia_layout_detached_begin(ctx, &data);
            test_leaf(ctx, alia_vec2f_make(30.f, 10.f), 0, &detached_box);
            alia_layout_detached_end(ctx);
            test_leaf(ctx, alia_vec2f_make(40.f, 20.f), 0, &main_box);
        });
    };

    layout_test_fixture_run_refresh(fixture, ui);

    REQUIRE(layout_fixture_root_child(fixture) != nullptr);

    layout_test_fixture_resolve(fixture, alia_vec2f_make(100.f, 50.f));
    layout_test_fixture_run_spatial(fixture, ui);

    CHECK(check_box_eq(
        main_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(40.f, 20.f)));

    CHECK(preferred.x == doctest::Approx(30.f));
    CHECK(preferred.y == doctest::Approx(10.f));
    CHECK(check_box_eq(
        detached_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(30.f, 10.f)));

    layout_test_fixture_destroy(fixture);
}

TEST_CASE("detached measure returns intrinsic size; assign can be smaller")
{
    alia_box detached_box{};
    alia_layout_detached data{};
    alia_vec2f preferred{};

    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [&](alia_context* ctx) {
        if (!alia_layout_context_is_refresh(ctx))
        {
            preferred = alia_layout_detached_measure(ctx, &data);
            // A popup helper clamps preferred to the space that remains.
            alia_layout_detached_assign(
                ctx, &data, alia_vec2f_make(50.f, 40.f));
        }
        alia_layout_detached_begin(ctx, &data);
        test_leaf(ctx, alia_vec2f_make(80.f, 60.f), ALIA_FILL, &detached_box);
        alia_layout_detached_end(ctx);
    };

    layout_test_fixture_run_refresh(fixture, ui);
    layout_test_fixture_resolve(fixture, alia_vec2f_make(200.f, 200.f));
    layout_test_fixture_run_spatial(fixture, ui);

    CHECK(preferred.x == doctest::Approx(80.f));
    CHECK(preferred.y == doctest::Approx(60.f));
    CHECK(check_box_eq(
        detached_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(50.f, 40.f)));

    layout_test_fixture_destroy(fixture);
}

TEST_CASE("detached measure reports content size for shrinkable widgets")
{
    alia_layout_detached data{};
    alia_vec2f preferred{};
    bool overflow = false;

    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [&](alia_context* ctx) {
        if (alia_layout_context_is_refresh(ctx))
        {
            alia_layout_detached_begin(ctx, &data);
            emit_shrinkable(ctx, &overflow);
            alia_layout_detached_end(ctx);
            return;
        }

        preferred = alia_layout_detached_measure(ctx, &data);
        alia_layout_detached_assign(ctx, &data, alia_vec2f_make(40.f, 30.f));
    };

    layout_test_fixture_run_refresh(fixture, ui);
    layout_test_fixture_resolve(fixture, alia_vec2f_make(200.f, 200.f));
    layout_test_fixture_run_spatial(fixture, ui);

    CHECK(preferred.x == doctest::Approx(40.f));
    CHECK(preferred.y == doctest::Approx(shrinkable_content_height));
    CHECK(overflow);

    layout_test_fixture_destroy(fixture);
}

TEST_CASE("detached assign uses the caller-provided size")
{
    alia_box detached_box{};
    alia_layout_detached data{};

    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [&](alia_context* ctx) {
        if (!alia_layout_context_is_refresh(ctx))
        {
            // Preferred is smaller than the assigned box; assign must still
            // use the exact size (e.g. a popup min-width floor).
            alia_vec2f const preferred
                = alia_layout_detached_measure(ctx, &data);
            CHECK(preferred.x == doctest::Approx(30.f));
            alia_layout_detached_assign(
                ctx, &data, alia_vec2f_make(80.f, 40.f));
        }
        alia_layout_detached_begin(ctx, &data);
        test_leaf(ctx, alia_vec2f_make(30.f, 10.f), ALIA_FILL, &detached_box);
        alia_layout_detached_end(ctx);
    };

    layout_test_fixture_run_refresh(fixture, ui);
    layout_test_fixture_resolve(fixture, alia_vec2f_make(200.f, 200.f));
    layout_test_fixture_run_spatial(fixture, ui);

    CHECK(check_box_eq(
        detached_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(80.f, 40.f)));
    CHECK(alia_layout_detached_size(&data).x == doctest::Approx(80.f));

    layout_test_fixture_destroy(fixture);
}

TEST_CASE("detached layout supports nesting")
{
    alia_box outer_box{};
    alia_box inner_box{};
    alia_layout_detached outer{};
    alia_layout_detached inner{};
    alia_vec2f outer_preferred{};
    alia_vec2f inner_preferred{};

    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [&](alia_context* ctx) {
        if (!alia_layout_context_is_refresh(ctx))
        {
            outer_preferred = alia_layout_detached_measure(ctx, &outer);
            alia_layout_detached_assign(ctx, &outer, outer_preferred);
        }
        alia_layout_detached_begin(ctx, &outer);
        column(ctx, [&]() {
            test_leaf(ctx, alia_vec2f_make(20.f, 10.f), 0, &outer_box);
            if (!alia_layout_context_is_refresh(ctx))
            {
                inner_preferred = alia_layout_detached_measure(ctx, &inner);
                alia_layout_detached_assign(ctx, &inner, inner_preferred);
            }
            alia_layout_detached_begin(ctx, &inner);
            test_leaf(ctx, alia_vec2f_make(15.f, 8.f), 0, &inner_box);
            alia_layout_detached_end(ctx);
        });
        alia_layout_detached_end(ctx);
    };

    layout_test_fixture_run_refresh(fixture, ui);
    layout_test_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));
    layout_test_fixture_run_spatial(fixture, ui);

    CHECK(outer_preferred.x == doctest::Approx(20.f));
    CHECK(outer_preferred.y == doctest::Approx(10.f));
    CHECK(inner_preferred.x == doctest::Approx(15.f));
    CHECK(inner_preferred.y == doctest::Approx(8.f));
    CHECK(check_box_eq(
        outer_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    CHECK(check_box_eq(
        inner_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(15.f, 8.f)));

    layout_test_fixture_destroy(fixture);
}

TEST_CASE("detached assign is skipped within the same placement generation")
{
    alia_layout_detached data{};
    alia_box box{};

    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    layout_test_fixture_run_refresh(fixture, [&](alia_context* ctx) {
        alia_layout_detached_begin(ctx, &data);
        test_leaf(ctx, alia_vec2f_make(12.f, 6.f), 0, &box);
        alia_layout_detached_end(ctx);
    });
    layout_test_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));

    alia_offset first_offset = 0;
    layout_test_fixture_run_spatial(fixture, [&](alia_context* ctx) {
        alia_layout_detached_assign(ctx, &data, alia_vec2f_make(12.f, 6.f));
        first_offset = data.region.offset;
        alia_layout_detached_begin(ctx, &data);
        test_leaf(ctx, alia_vec2f_make(12.f, 6.f), 0, &box);
        alia_layout_detached_end(ctx);

        // A second assign in the same generation must not append again.
        alia_layout_detached_assign(ctx, &data, alia_vec2f_make(10.f, 10.f));
        CHECK(data.region.offset == first_offset);
        CHECK(data.size.x == doctest::Approx(12.f));
        CHECK(data.size.y == doctest::Approx(6.f));
    });

    layout_test_fixture_destroy(fixture);
}

TEST_CASE("placement clear advances generation for detached re-assign")
{
    alia_layout_detached data{};
    alia_box box{};

    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [&](alia_context* ctx) {
        if (!alia_layout_context_is_refresh(ctx))
        {
            alia_layout_detached_assign(
                ctx, &data, alia_vec2f_make(10.f, 10.f));
        }
        alia_layout_detached_begin(ctx, &data);
        test_leaf(ctx, alia_vec2f_make(10.f, 10.f), 0, &box);
        alia_layout_detached_end(ctx);
    };

    layout_test_fixture_run_refresh(fixture, ui);
    layout_test_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));
    layout_test_fixture_run_spatial(fixture, ui);
    auto const generation_after_first = data.generation;

    layout_test_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));
    CHECK(
        alia_layout_placement_current_generation(
            layout_fixture_layout_system(fixture))
        != generation_after_first);

    layout_test_fixture_run_spatial(fixture, ui);
    CHECK(data.generation != generation_after_first);

    layout_test_fixture_destroy(fixture);
}
