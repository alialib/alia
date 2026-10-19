#include <alia/test/layout/layout_fixture.hpp>

#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/layout/detached.h>
#include <alia/abi/ui/layout/placement.h>
#include <alia/abi/ui/layout/utilities/defaults.h>
#include <alia/abi/ui/layout/utilities/emission.h>

#include <string.h>

#define TEST_NO_MAIN
#include "acutest.h"

static float const shrinkable_content_height = 80.f;
static float const shrinkable_minimum_height = 20.f;

typedef struct shrinkable_node
{
    alia_layout_node base;
    bool* overflow;
} shrinkable_node;

static alia_horizontal_requirements
shrinkable_measure_horizontal(
    alia_measurement_context* ctx, alia_main_axis_index axis, alia_layout_node* node)
{
    (void) ctx;
    (void) axis;
    (void) node;
    alia_horizontal_requirements requirements;
    requirements.min_size = 40.f;
    requirements.growth_factor = 0.f;
    return requirements;
}

static alia_vertical_requirements
shrinkable_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index axis,
    alia_layout_node* node,
    float assigned_width)
{
    (void) axis;
    (void) node;
    (void) assigned_width;
    float const height = ctx->measure_preferred_size
                           ? shrinkable_content_height
                           : shrinkable_minimum_height;
    alia_vertical_requirements requirements;
    requirements.min_size = height;
    requirements.growth_factor = 0.f;
    requirements.ascent = height;
    requirements.descent = 0.f;
    return requirements;
}

static void
shrinkable_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    (void) ctx;
    (void) axis;
    (void) baseline;
    shrinkable_node* n = (shrinkable_node*) node;
    if (n->overflow)
        *n->overflow = box.size.y < shrinkable_content_height;
}

static alia_layout_node_vtable shrinkable_vtable
    = {shrinkable_measure_horizontal,
       shrinkable_measure_vertical,
       shrinkable_assign_boxes,
       alia_default_count_flow_emissions,
       alia_default_emit_flow_fragments,
       alia_default_read_fragment_placements};

static void
emit_shrinkable(alia_ui_context* ctx, bool* overflow)
{
    alia_bump_allocator* arena = &ctx->layout->emission.arena;
    shrinkable_node* node = (shrinkable_node*) alia_arena_ptr(
        arena,
        alia_arena_alloc(arena, alia_min_aligned_size(sizeof(shrinkable_node))));
    node->base.vtable = &shrinkable_vtable;
    node->base.next_sibling = NULL;
    node->overflow = overflow;
    *ctx->layout->emission.next_ptr = &node->base;
    ctx->layout->emission.next_ptr = &node->base.next_sibling;
}

static int
check_box_eq(alia_box box, alia_vec2f min, alia_vec2f size)
{
    return alia_box_equal(box, alia_box_make(min, size));
}

static int
near(float a, float b)
{
    return a > b - 0.001f && a < b + 0.001f;
}

static void
test_leaf(
    alia_ui_context* ctx, alia_vec2f size, alia_layout_flags_t flags, alia_box* out)
{
    if (alia_layout_context_is_refresh(ctx))
    {
        alia_layout_content_metrics metrics;
        metrics.size = size;
        metrics.ascent = 0.f;
        metrics.descent = 0.f;
        alia_layout_leaf_emit(ctx, metrics, flags);
    }
    else
    {
        alia_box box = alia_layout_consume_box(ctx);
        if (out)
            *out = box;
    }
}

typedef struct separate_state
{
    alia_box main_box;
    alia_box detached_box;
    alia_layout_detached data;
    alia_vec2f preferred;
} separate_state;

static void
separate_ui(alia_ui_context* ctx, void* user)
{
    separate_state* state = (separate_state*) user;
    alia_layout_row_begin(ctx, 0, 0.f);
    if (!alia_layout_context_is_refresh(ctx))
    {
        state->preferred = alia_layout_detached_measure(ctx, &state->data);
        alia_layout_detached_assign(ctx, &state->data, state->preferred);
    }
    alia_layout_detached_begin(ctx, &state->data);
    test_leaf(ctx, alia_vec2f_make(30.f, 10.f), 0, &state->detached_box);
    alia_layout_detached_end(ctx);
    test_leaf(ctx, alia_vec2f_make(40.f, 20.f), 0, &state->main_box);
    alia_layout_row_end(ctx);
}

static void
test_assigns_separately_from_the_main_tree(void)
{
    separate_state state;
    memset(&state, 0, sizeof(state));
    struct layout_fixture* fixture = layout_fixture_create();
    TEST_ASSERT(fixture != NULL);

    layout_fixture_run_refresh_impl(fixture, separate_ui, &state);
    TEST_CHECK(layout_fixture_root_child(fixture) != NULL);
    layout_fixture_resolve(fixture, alia_vec2f_make(100.f, 50.f));
    layout_fixture_run_spatial_impl(fixture, separate_ui, &state);

    TEST_CHECK(check_box_eq(
        state.main_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(40.f, 20.f)));
    TEST_CHECK(near(state.preferred.x, 30.f));
    TEST_CHECK(near(state.preferred.y, 10.f));
    TEST_CHECK(check_box_eq(
        state.detached_box,
        alia_vec2f_make(0.f, 0.f),
        alia_vec2f_make(30.f, 10.f)));

    layout_fixture_destroy(fixture);
}

typedef struct intrinsic_state
{
    alia_box detached_box;
    alia_layout_detached data;
    alia_vec2f preferred;
} intrinsic_state;

static void
intrinsic_ui(alia_ui_context* ctx, void* user)
{
    intrinsic_state* state = (intrinsic_state*) user;
    if (!alia_layout_context_is_refresh(ctx))
    {
        state->preferred = alia_layout_detached_measure(ctx, &state->data);
        alia_layout_detached_assign(ctx, &state->data, alia_vec2f_make(50.f, 40.f));
    }
    alia_layout_detached_begin(ctx, &state->data);
    test_leaf(ctx, alia_vec2f_make(80.f, 60.f), ALIA_FILL, &state->detached_box);
    alia_layout_detached_end(ctx);
}

static void
test_measure_returns_intrinsic_size(void)
{
    intrinsic_state state;
    memset(&state, 0, sizeof(state));
    struct layout_fixture* fixture = layout_fixture_create();
    TEST_ASSERT(fixture != NULL);

    layout_fixture_run_refresh_impl(fixture, intrinsic_ui, &state);
    layout_fixture_resolve(fixture, alia_vec2f_make(200.f, 200.f));
    layout_fixture_run_spatial_impl(fixture, intrinsic_ui, &state);

    TEST_CHECK(near(state.preferred.x, 80.f));
    TEST_CHECK(near(state.preferred.y, 60.f));
    TEST_CHECK(check_box_eq(
        state.detached_box,
        alia_vec2f_make(0.f, 0.f),
        alia_vec2f_make(50.f, 40.f)));

    layout_fixture_destroy(fixture);
}

typedef struct shrinkable_state
{
    alia_layout_detached data;
    alia_vec2f preferred;
    bool overflow;
} shrinkable_state;

static void
shrinkable_ui(alia_ui_context* ctx, void* user)
{
    shrinkable_state* state = (shrinkable_state*) user;
    if (alia_layout_context_is_refresh(ctx))
    {
        alia_layout_detached_begin(ctx, &state->data);
        emit_shrinkable(ctx, &state->overflow);
        alia_layout_detached_end(ctx);
        return;
    }

    state->preferred = alia_layout_detached_measure(ctx, &state->data);
    alia_layout_detached_assign(ctx, &state->data, alia_vec2f_make(40.f, 30.f));
}

static void
test_measure_reports_content_size_for_shrinkable_widgets(void)
{
    shrinkable_state state;
    memset(&state, 0, sizeof(state));
    struct layout_fixture* fixture = layout_fixture_create();
    TEST_ASSERT(fixture != NULL);

    layout_fixture_run_refresh_impl(fixture, shrinkable_ui, &state);
    layout_fixture_resolve(fixture, alia_vec2f_make(200.f, 200.f));
    layout_fixture_run_spatial_impl(fixture, shrinkable_ui, &state);

    TEST_CHECK(near(state.preferred.x, 40.f));
    TEST_CHECK(near(state.preferred.y, shrinkable_content_height));
    TEST_CHECK(state.overflow);

    layout_fixture_destroy(fixture);
}

typedef struct caller_size_state
{
    alia_box detached_box;
    alia_layout_detached data;
} caller_size_state;

static void
caller_size_ui(alia_ui_context* ctx, void* user)
{
    caller_size_state* state = (caller_size_state*) user;
    if (!alia_layout_context_is_refresh(ctx))
    {
        alia_vec2f preferred = alia_layout_detached_measure(ctx, &state->data);
        TEST_CHECK(near(preferred.x, 30.f));
        alia_layout_detached_assign(ctx, &state->data, alia_vec2f_make(80.f, 40.f));
    }
    alia_layout_detached_begin(ctx, &state->data);
    test_leaf(ctx, alia_vec2f_make(30.f, 10.f), ALIA_FILL, &state->detached_box);
    alia_layout_detached_end(ctx);
}

static void
test_assign_uses_the_caller_provided_size(void)
{
    caller_size_state state;
    memset(&state, 0, sizeof(state));
    struct layout_fixture* fixture = layout_fixture_create();
    TEST_ASSERT(fixture != NULL);

    layout_fixture_run_refresh_impl(fixture, caller_size_ui, &state);
    layout_fixture_resolve(fixture, alia_vec2f_make(200.f, 200.f));
    layout_fixture_run_spatial_impl(fixture, caller_size_ui, &state);

    TEST_CHECK(check_box_eq(
        state.detached_box,
        alia_vec2f_make(0.f, 0.f),
        alia_vec2f_make(80.f, 40.f)));
    TEST_CHECK(near(alia_layout_detached_size(&state.data).x, 80.f));

    layout_fixture_destroy(fixture);
}

typedef struct nesting_state
{
    alia_box outer_box;
    alia_box inner_box;
    alia_layout_detached outer;
    alia_layout_detached inner;
    alia_vec2f outer_preferred;
    alia_vec2f inner_preferred;
} nesting_state;

static void
nesting_ui(alia_ui_context* ctx, void* user)
{
    nesting_state* state = (nesting_state*) user;
    if (!alia_layout_context_is_refresh(ctx))
    {
        state->outer_preferred = alia_layout_detached_measure(ctx, &state->outer);
        alia_layout_detached_assign(ctx, &state->outer, state->outer_preferred);
    }
    alia_layout_detached_begin(ctx, &state->outer);
    alia_layout_column_begin(ctx, 0, 0.f);
    test_leaf(ctx, alia_vec2f_make(20.f, 10.f), 0, &state->outer_box);
    if (!alia_layout_context_is_refresh(ctx))
    {
        state->inner_preferred = alia_layout_detached_measure(ctx, &state->inner);
        alia_layout_detached_assign(ctx, &state->inner, state->inner_preferred);
    }
    alia_layout_detached_begin(ctx, &state->inner);
    test_leaf(ctx, alia_vec2f_make(15.f, 8.f), 0, &state->inner_box);
    alia_layout_detached_end(ctx);
    alia_layout_column_end(ctx);
    alia_layout_detached_end(ctx);
}

static void
test_supports_nesting(void)
{
    nesting_state state;
    memset(&state, 0, sizeof(state));
    struct layout_fixture* fixture = layout_fixture_create();
    TEST_ASSERT(fixture != NULL);

    layout_fixture_run_refresh_impl(fixture, nesting_ui, &state);
    layout_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));
    layout_fixture_run_spatial_impl(fixture, nesting_ui, &state);

    TEST_CHECK(near(state.outer_preferred.x, 20.f));
    TEST_CHECK(near(state.outer_preferred.y, 10.f));
    TEST_CHECK(near(state.inner_preferred.x, 15.f));
    TEST_CHECK(near(state.inner_preferred.y, 8.f));
    TEST_CHECK(check_box_eq(
        state.outer_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(20.f, 10.f)));
    TEST_CHECK(check_box_eq(
        state.inner_box, alia_vec2f_make(0.f, 0.f), alia_vec2f_make(15.f, 8.f)));

    layout_fixture_destroy(fixture);
}

typedef struct same_generation_state
{
    alia_layout_detached data;
    alia_box box;
    alia_offset first_offset;
    int saw_spatial;
} same_generation_state;

static void
same_generation_refresh(alia_ui_context* ctx, void* user)
{
    same_generation_state* state = (same_generation_state*) user;
    alia_layout_detached_begin(ctx, &state->data);
    test_leaf(ctx, alia_vec2f_make(12.f, 6.f), 0, &state->box);
    alia_layout_detached_end(ctx);
}

static void
same_generation_spatial(alia_ui_context* ctx, void* user)
{
    same_generation_state* state = (same_generation_state*) user;
    alia_layout_detached_assign(ctx, &state->data, alia_vec2f_make(12.f, 6.f));
    state->first_offset = state->data.region.offset;
    alia_layout_detached_begin(ctx, &state->data);
    test_leaf(ctx, alia_vec2f_make(12.f, 6.f), 0, &state->box);
    alia_layout_detached_end(ctx);

    alia_layout_detached_assign(ctx, &state->data, alia_vec2f_make(10.f, 10.f));
    state->saw_spatial = 1;
    TEST_CHECK(state->data.region.offset == state->first_offset);
    TEST_CHECK(near(state->data.size.x, 12.f));
    TEST_CHECK(near(state->data.size.y, 6.f));
}

static void
test_assign_is_skipped_within_the_same_generation(void)
{
    same_generation_state state;
    memset(&state, 0, sizeof(state));
    struct layout_fixture* fixture = layout_fixture_create();
    TEST_ASSERT(fixture != NULL);

    layout_fixture_run_refresh_impl(fixture, same_generation_refresh, &state);
    layout_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));
    layout_fixture_run_spatial_impl(fixture, same_generation_spatial, &state);
    TEST_CHECK(state.saw_spatial);

    layout_fixture_destroy(fixture);
}

typedef struct generation_state
{
    alia_layout_detached data;
    alia_box box;
} generation_state;

static void
generation_ui(alia_ui_context* ctx, void* user)
{
    generation_state* state = (generation_state*) user;
    if (!alia_layout_context_is_refresh(ctx))
    {
        alia_layout_detached_assign(
            ctx, &state->data, alia_vec2f_make(10.f, 10.f));
    }
    alia_layout_detached_begin(ctx, &state->data);
    test_leaf(ctx, alia_vec2f_make(10.f, 10.f), 0, &state->box);
    alia_layout_detached_end(ctx);
}

static void
test_placement_clear_advances_generation(void)
{
    generation_state state;
    memset(&state, 0, sizeof(state));
    struct layout_fixture* fixture = layout_fixture_create();
    TEST_ASSERT(fixture != NULL);

    layout_fixture_run_refresh_impl(fixture, generation_ui, &state);
    layout_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));
    layout_fixture_run_spatial_impl(fixture, generation_ui, &state);
    alia_layout_placement_generation const generation_after_first
        = state.data.generation;

    layout_fixture_resolve(fixture, alia_vec2f_make(100.f, 100.f));
    TEST_CHECK(
        alia_layout_placement_current_generation(
            layout_fixture_layout_system(fixture))
        != generation_after_first);

    layout_fixture_run_spatial_impl(fixture, generation_ui, &state);
    TEST_CHECK(state.data.generation != generation_after_first);

    layout_fixture_destroy(fixture);
}

void
layout_detached_tests(void)
{
    test_assigns_separately_from_the_main_tree();
    test_measure_returns_intrinsic_size();
    test_measure_reports_content_size_for_shrinkable_widgets();
    test_assign_uses_the_caller_provided_size();
    test_supports_nesting();
    test_assign_is_skipped_within_the_same_generation();
    test_placement_clear_advances_generation();
}
