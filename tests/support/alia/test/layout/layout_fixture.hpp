#pragma once

// Layout test/benchmark support. Not part of the public alia API.

#include <alia/abi/base/arena.h>
#include <alia/abi/context.h>
#include <alia/abi/ui/layout/system.h>

#ifdef __cplusplus
#include <type_traits>
#include <utility>
#endif

struct layout_fixture;
struct alia_layout_node;

#ifdef __cplusplus
extern "C" {
#endif

layout_fixture*
layout_fixture_create();

void
layout_fixture_destroy(layout_fixture* fixture);

alia_context*
layout_fixture_context(layout_fixture* fixture);

alia_layout_system*
layout_fixture_layout_system(layout_fixture* fixture);

alia_layout_node*
layout_fixture_root_child(layout_fixture* fixture);

alia_arena_stats
layout_fixture_node_arena_stats(layout_fixture* fixture);

void*
layout_fixture_placement_arena_identity(layout_fixture* fixture);

void
layout_fixture_set_spacing(layout_fixture* fixture, float spacing);

void
layout_fixture_run_refresh_impl(
    layout_fixture* fixture, void (*fn)(alia_context*, void*), void* user);

void
layout_fixture_resolve(layout_fixture* fixture, alia_vec2f available);

void
layout_fixture_run_spatial_impl(
    layout_fixture* fixture, void (*fn)(alia_context*, void*), void* user);

bool
alia_layout_context_is_refresh(alia_context* ctx);

#ifdef __cplusplus
} // extern "C"

template<class Fn>
void
layout_fixture_run_refresh(layout_fixture* fixture, Fn&& fn)
{
    layout_fixture_run_refresh_impl(
        fixture,
        [](alia_context* ctx, void* user) {
            (*static_cast<std::decay_t<Fn>*>(user))(ctx);
        },
        &fn);
}

template<class Fn>
void
layout_fixture_run_spatial(layout_fixture* fixture, Fn&& fn)
{
    layout_fixture_run_spatial_impl(
        fixture,
        [](alia_context* ctx, void* user) {
            (*static_cast<std::decay_t<Fn>*>(user))(ctx);
        },
        &fn);
}

// Backward-compatible aliases for layout tests.
using layout_test_fixture = layout_fixture;

inline layout_test_fixture*
layout_test_fixture_create()
{
    return layout_fixture_create();
}

inline void
layout_test_fixture_destroy(layout_test_fixture* fixture)
{
    layout_fixture_destroy(fixture);
}

inline alia_context*
layout_test_fixture_context(layout_test_fixture* fixture)
{
    return layout_fixture_context(fixture);
}

inline void
layout_test_fixture_set_spacing(layout_test_fixture* fixture, float spacing)
{
    layout_fixture_set_spacing(fixture, spacing);
}

inline void
layout_test_fixture_resolve(layout_test_fixture* fixture, alia_vec2f available)
{
    layout_fixture_resolve(fixture, available);
}

template<class Fn>
void
layout_test_fixture_run_refresh(layout_test_fixture* fixture, Fn&& fn)
{
    layout_fixture_run_refresh(fixture, std::forward<Fn>(fn));
}

template<class Fn>
void
layout_test_fixture_run_spatial(layout_test_fixture* fixture, Fn&& fn)
{
    layout_fixture_run_spatial(fixture, std::forward<Fn>(fn));
}

#endif // __cplusplus
