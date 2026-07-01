#pragma once

#include "bench_common.hpp"

#include <alia/test/layout/layout_fixture.hpp>
#include <alia/test/layout/scenarios.h>

#include <cassert>
#include <string>

struct layout_bench_scenario
{
    char const* name;
    alia_layout_scenario_fn fn;
    alia_vec2f available;
    uint32_t (*leaf_count)();
};

inline void
assert_scenario_sanity(
    layout_fixture* fixture, layout_bench_scenario const& scenario)
{
    assert(layout_fixture_root_child(fixture) != nullptr);

    alia_arena_stats stats = layout_fixture_node_arena_stats(fixture);
    assert(stats.current_usage > 0);
    (void) scenario.leaf_count;
}

inline void
bench_layout_phases(
    ankerl::nanobench::Bench& suite,
    layout_fixture* fixture,
    layout_bench_scenario const& scenario)
{
    std::string const prefix = std::string(scenario.name) + "/";

    layout_fixture_run_refresh_impl(fixture, scenario.fn, nullptr);
    assert_scenario_sanity(fixture, scenario);

    suite.run(prefix + "refresh", [&] {
        layout_fixture_run_refresh_impl(fixture, scenario.fn, nullptr);
        ankerl::nanobench::doNotOptimizeAway(
            layout_fixture_root_child(fixture));
    });

    layout_fixture_run_refresh_impl(fixture, scenario.fn, nullptr);
    suite.run(prefix + "resolve", [&] {
        layout_fixture_resolve(fixture, scenario.available);
        ankerl::nanobench::doNotOptimizeAway(
            layout_fixture_placement_arena_identity(fixture));
    });

    layout_fixture_run_refresh_impl(fixture, scenario.fn, nullptr);
    layout_fixture_resolve(fixture, scenario.available);
    suite.run(prefix + "spatial", [&] {
        layout_fixture_run_spatial_impl(fixture, scenario.fn, nullptr);
        ankerl::nanobench::doNotOptimizeAway(layout_fixture_context(fixture));
    });
}
