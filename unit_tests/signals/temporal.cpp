#include <alia/signals/temporal.hpp>

#include <catch.hpp>

#include <thread>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("animation_timer", "[components][system]")
{
    alia::system sys;

    set_automatic_time_updates(sys, false);
    set_millisecond_tick_counter(sys, 0);

    REQUIRE(!system_needs_refresh(sys));

    do_traversal(sys, [&](context ctx) {
        animation_timer timer(ctx);
        auto active = timer.is_active();
        REQUIRE(signal_is_readable(active));
        REQUIRE(!read_signal(active));
        auto start = timer.start(100);
        REQUIRE(action_is_ready(start));
        perform_action(start);
    });

    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 10);

    do_traversal(sys, [&](context ctx) {
        animation_timer_state* state;
        get_cached_data(ctx, &state);
        animation_timer timer(ctx, *state);
        auto active = timer.is_active();
        REQUIRE(signal_is_readable(active));
        REQUIRE(read_signal(active));
        auto ticks = timer.ticks_left();
        REQUIRE(signal_is_readable(ticks));
        REQUIRE(read_signal(ticks) == 90);
    });

    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 110);

    do_traversal(sys, [&](context ctx) {
        animation_timer timer(ctx);
        auto active = timer.is_active();
        REQUIRE(signal_is_readable(active));
        REQUIRE(!read_signal(active));
        timer.start(100);
    });

    REQUIRE(!system_needs_refresh(sys));
}
