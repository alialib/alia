#include <alia/system/internals.hpp>

#include <testing.hpp>

#include <thread>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("automatic ticks", "[system]")
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
    millisecond_count tick_count = 0;

    void
    schedule_animation_refresh()
    {
        refresh_requested = true;
    }

    millisecond_count
    get_tick_count() const
    {
        return tick_count;
    }
};

TEST_CASE("get_raw_animation_ticks_left", "[system]")
{
    alia::system sys;
    dummy_external_interface external;
    sys.external = &external;

    REQUIRE(!system_needs_refresh(sys));

    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_ticks_left(ctx, 100) == 100);
    });

    REQUIRE(system_needs_refresh(sys));
    REQUIRE(external.refresh_requested);
    external.refresh_requested = false;

    external.tick_count = 10;

    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_ticks_left(ctx, 100) == 90);
    });

    REQUIRE(system_needs_refresh(sys));
    REQUIRE(external.refresh_requested);
    external.refresh_requested = false;

    external.tick_count = 110;

    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_ticks_left(ctx, 100) == 0);
    });

    REQUIRE(!system_needs_refresh(sys));
    REQUIRE(!external.refresh_requested);
}

TEST_CASE("animation_timer", "[signals][temporal]")
{
    alia::system sys;
    dummy_external_interface external;
    sys.external = &external;

    REQUIRE(!system_needs_refresh(sys));

    do_traversal(sys, [&](context ctx) {
        animation_timer timer(ctx);
        auto active = timer.is_active();
        REQUIRE(signal_has_value(active));
        REQUIRE(!read_signal(active));
        auto start = timer.start() << 100;
        REQUIRE(action_is_ready(start));
        perform_action(start);
    });

    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 10;

    do_traversal(sys, [&](context ctx) {
        animation_timer_state* state;
        get_cached_data(ctx, &state);
        animation_timer timer(ctx, *state);
        auto active = timer.is_active();
        REQUIRE(signal_has_value(active));
        REQUIRE(read_signal(active));
        auto ticks = timer.ticks_left();
        REQUIRE(signal_has_value(ticks));
        REQUIRE(read_signal(ticks) == 90);
    });

    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 110;

    do_traversal(sys, [&](context ctx) {
        animation_timer timer(ctx);
        auto active = timer.is_active();
        REQUIRE(signal_has_value(active));
        REQUIRE(!read_signal(active));
    });

    REQUIRE(!system_needs_refresh(sys));
}
