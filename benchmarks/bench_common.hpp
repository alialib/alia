#pragma once

#include "nanobench.h"

#include <chrono>
#include <cstdlib>

inline bool
benchmark_smoke_mode()
{
    char const* value = std::getenv("ALIA_BENCHMARK_SMOKE");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

inline ankerl::nanobench::Bench
make_bench()
{
    ankerl::nanobench::Bench bench;
    if (benchmark_smoke_mode())
    {
        bench.minEpochIterations(1)
            .minEpochTime(std::chrono::nanoseconds(1))
            .epochs(1);
    }
    else
        bench.minEpochIterations(1000);
    return bench;
}
