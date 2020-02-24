#include <alia/components/system.hpp>

#include <testing.hpp>

#include <thread>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("manual ticks", "[components][system]")
{
    alia::system sys;
    set_automatic_time_updates(sys, false);
    millisecond_count expected_tick_count = 0;
    set_millisecond_tick_counter(sys, expected_tick_count);
    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_tick_count(ctx) == expected_tick_count);
        auto ticks = get_animation_tick_count(ctx);
        REQUIRE(signal_has_value(ticks));
        REQUIRE(read_signal(ticks) == expected_tick_count);
    });
    expected_tick_count = 1;
    set_millisecond_tick_counter(sys, expected_tick_count);
    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_tick_count(ctx) == expected_tick_count);
        auto ticks = get_animation_tick_count(ctx);
        REQUIRE(signal_has_value(ticks));
        REQUIRE(read_signal(ticks) == expected_tick_count);
    });
}

TEST_CASE("automatic ticks", "[components][system]")
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

struct dummy_external_interface : external_interface
{
    bool refresh_requested = false;

    void
    request_animation_refresh()
    {
        refresh_requested = true;
    }
};

TEST_CASE("get_raw_animation_ticks_left", "[components][system]")
{
    alia::system sys;
    dummy_external_interface external;
    sys.external = &external;

    set_automatic_time_updates(sys, false);
    set_millisecond_tick_counter(sys, 0);

    REQUIRE(!system_needs_refresh(sys));

    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_ticks_left(ctx, 100) == 100);
    });

    REQUIRE(system_needs_refresh(sys));
    REQUIRE(external.refresh_requested);
    external.refresh_requested = false;

    set_millisecond_tick_counter(sys, 10);

    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_ticks_left(ctx, 100) == 90);
    });

    REQUIRE(system_needs_refresh(sys));
    REQUIRE(external.refresh_requested);
    external.refresh_requested = false;

    set_millisecond_tick_counter(sys, 110);

    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_ticks_left(ctx, 100) == 0);
    });

    REQUIRE(!system_needs_refresh(sys));
    REQUIRE(!external.refresh_requested);
}
