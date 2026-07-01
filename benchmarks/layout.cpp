#define ANKERL_NANOBENCH_IMPLEMENT
#include "layout_bench_phases.hpp"

#include <fstream>
#include <iostream>

int
main()
{
    layout_fixture* fixture = layout_fixture_create();
    if (!fixture)
        return 1;

    layout_bench_scenario const scenarios[] = {
        {"nested_grid_10x10x10",
         alia_layout_scenario_nested_grid_10,
         alia_vec2f_make(10'000.f, 10'000.f),
         alia_layout_scenario_nested_grid_10_leaf_count},
        {"column_of_rows_100",
         alia_layout_scenario_column_of_rows_100,
         alia_vec2f_make(10'000.f, 10'000.f),
         alia_layout_scenario_column_of_rows_100_leaf_count},
        {"growth_rows_100",
         alia_layout_scenario_growth_rows_100,
         alia_vec2f_make(10'000.f, 10'000.f),
         alia_layout_scenario_growth_rows_100_leaf_count},
    };

    ankerl::nanobench::Bench suite = make_bench();
    for (auto const& scenario : scenarios)
        bench_layout_phases(suite, fixture, scenario);

    ankerl::nanobench::render(
        ankerl::nanobench::templates::csv(), suite, std::cout);
    if (!benchmark_smoke_mode())
    {
        std::ofstream json_out("layout_benchmark_results.json");
        suite.render(ankerl::nanobench::templates::json(), json_out);
    }

    layout_fixture_destroy(fixture);
    return 0;
}
