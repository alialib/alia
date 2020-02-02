#include <alia/components/system.hpp>

#include <catch.hpp>

#include <thread>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("manual ticks", "[signals][temporal]")
{
    alia::system sys;
    set_automatic_time_updates(sys, false);
    millisecond_count expected_tick_count = 0;
    set_millisecond_tick_counter(sys, expected_tick_count);
    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_tick_count(ctx) == expected_tick_count);
        auto ticks = get_animation_tick_count(ctx);
        REQUIRE(signal_is_readable(ticks));
        REQUIRE(read_signal(ticks) == expected_tick_count);
    });
    expected_tick_count = 1;
    set_millisecond_tick_counter(sys, expected_tick_count);
    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_tick_count(ctx) == expected_tick_count);
        auto ticks = get_animation_tick_count(ctx);
        REQUIRE(signal_is_readable(ticks));
        REQUIRE(read_signal(ticks) == expected_tick_count);
    });
}

TEST_CASE("automatic ticks", "[signals][temporal]")
{
    alia::system sys;
    millisecond_count last_ticks;
    do_traversal(sys, [&](context ctx) {
        last_ticks = get_raw_animation_tick_count(ctx);
    });
    std::this_thread::sleep_for(std::chrono::seconds(2));
    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_tick_count(ctx) != last_ticks);
    });
}
