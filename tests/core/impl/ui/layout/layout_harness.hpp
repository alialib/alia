#pragma once

#include <alia/abi/base/geometry/box.h>
#include <alia/abi/context.h>
#include <alia/abi/ui/layout/components.h>
#include <alia/context.hpp>
#include <alia/impl/events.hpp>
#include <alia/ui/layout/flags.hpp>

struct layout_test_fixture;

layout_test_fixture*
layout_test_fixture_create();

void
layout_test_fixture_destroy(layout_test_fixture* fixture);

alia_context*
layout_test_fixture_context(layout_test_fixture* fixture);

#pragma once

#include <alia/abi/base/geometry/box.h>
#include <alia/abi/context.h>
#include <alia/abi/ui/layout/components.h>
#include <alia/context.hpp>
#include <alia/impl/events.hpp>
#include <alia/ui/layout/flags.hpp>

#include <utility>

struct layout_test_fixture;

layout_test_fixture*
layout_test_fixture_create();

void
layout_test_fixture_destroy(layout_test_fixture* fixture);

alia_context*
layout_test_fixture_context(layout_test_fixture* fixture);

void
layout_test_fixture_run_refresh_impl(
    layout_test_fixture* fixture, void (*fn)(alia_context*, void*), void* user);

void
layout_test_fixture_resolve(layout_test_fixture* fixture, alia_vec2f available);

void
layout_test_fixture_run_spatial_impl(
    layout_test_fixture* fixture, void (*fn)(alia_context*, void*), void* user);

template<class Fn>
void
layout_test_fixture_run_refresh(layout_test_fixture* fixture, Fn&& fn)
{
    layout_test_fixture_run_refresh_impl(
        fixture,
        [](alia_context* ctx, void* user) {
            (*static_cast<std::decay_t<Fn>*>(user))(ctx);
        },
        &fn);
}

template<class Fn>
void
layout_test_fixture_run_spatial(layout_test_fixture* fixture, Fn&& fn)
{
    layout_test_fixture_run_spatial_impl(
        fixture,
        [](alia_context* ctx, void* user) {
            (*static_cast<std::decay_t<Fn>*>(user))(ctx);
        },
        &fn);
}

namespace alia::layout_test {

inline bool
check_box_eq(alia_box box, alia_vec2f min, alia_vec2f size)
{
    return alia_box_equal(box, alia_box_make(min, size));
}

inline void
test_leaf(
    alia_context& ctx,
    alia_vec2f size,
    layout_flag_set flags = NO_FLAGS,
    alia_box* out = nullptr)
{
    if (is_refresh_event(ctx))
        alia_layout_leaf_emit(&ctx, size, raw_code(flags));
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

} // namespace alia::layout_test
