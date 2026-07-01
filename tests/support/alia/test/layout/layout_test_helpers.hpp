#pragma once

#include <alia/test/layout/layout_fixture.hpp>

#include <alia/abi/base/geometry/box.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/context.hpp>
#include <alia/impl/events.hpp>
#include <alia/ui/layout/flags.hpp>

#include <cassert>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace alia::layout_test {

inline bool
check_box_eq(alia_box box, alia_vec2f min, alia_vec2f size)
{
    return alia_box_equal(box, alia_box_make(min, size));
}

inline bool
check_box_near(
    alia_box box, alia_vec2f min, alia_vec2f size, float eps = 1e-4f)
{
    return alia_vec2f_near(box.min, min, eps)
        && alia_vec2f_near(box.size, size, eps);
}

inline bool
check_boxes_eq(
    alia_layout_box_array boxes,
    std::initializer_list<std::pair<alia_vec2f, alia_vec2f>> expected)
{
    if (boxes.count != expected.size())
        return false;
    std::size_t i = 0;
    for (auto const& entry : expected)
    {
        if (!check_box_eq(boxes.boxes[i], entry.first, entry.second))
            return false;
        ++i;
    }
    return true;
}

inline void
test_leaf(
    alia_context& ctx,
    alia_vec2f size,
    layout_flag_set flags = NO_FLAGS,
    alia_box* out = nullptr,
    float ascent = 0.f,
    float descent = 0.f)
{
    if (is_refresh_event(ctx))
        alia_layout_leaf_emit(
            &ctx,
            alia_layout_content_metrics{
                .size = size, .ascent = ascent, .descent = descent},
            raw_code(flags));
    else
    {
        alia_box box = alia_layout_consume_box(&ctx);
        if (out)
            *out = box;
    }
}

inline void
test_spacer(alia_context& ctx, alia_vec2f size)
{
    test_leaf(ctx, size, NO_FLAGS);
}

inline void
test_flow_spring(alia_context& ctx, float min_width = 0.f)
{
    if (is_refresh_event(ctx))
        alia_layout_flow_spring_emit(&ctx, min_width);
    else
        (void) alia_layout_consume_box(&ctx);
}

template<class Fn>
void
run_layout_case(alia_vec2f assigned, Fn&& fn)
{
    layout_test_fixture* fixture = layout_test_fixture_create();
    assert(fixture != nullptr);

    layout_test_fixture_run_refresh(
        fixture, [&](alia_context* ctx) { fn(*ctx); });
    layout_test_fixture_resolve(fixture, assigned);
    layout_test_fixture_run_spatial(
        fixture, [&](alia_context* ctx) { fn(*ctx); });

    layout_test_fixture_destroy(fixture);
}

template<class Fn>
void
run_layout_case_with_spacing(float spacing, alia_vec2f assigned, Fn&& fn)
{
    layout_test_fixture* fixture = layout_test_fixture_create();
    assert(fixture != nullptr);
    layout_test_fixture_set_spacing(fixture, spacing);

    layout_test_fixture_run_refresh(
        fixture, [&](alia_context* ctx) { fn(*ctx); });
    layout_test_fixture_resolve(fixture, assigned);
    layout_test_fixture_run_spatial(
        fixture, [&](alia_context* ctx) { fn(*ctx); });

    layout_test_fixture_destroy(fixture);
}

} // namespace alia::layout_test
