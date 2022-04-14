#include <alia/system/internals.hpp>

#include <thread>

#include <alia/actions/operators.hpp>

#include <testing.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("automatic ticks", "[timing][ticks]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});
    millisecond_count last_ticks;
    do_traversal(sys, [&](context ctx) {
        last_ticks = get_raw_animation_tick_count(ctx);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    do_traversal(sys, [&](context ctx) {
        REQUIRE(get_raw_animation_tick_count(ctx) != last_ticks);
    });
}

struct dummy_external_interface : default_external_interface
{
    dummy_external_interface(alia::system& sys)
        : default_external_interface(sys)
    {
    }

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

TEST_CASE("get_raw_animation_ticks_left", "[timing][ticks]")
{
    alia::system sys;
    auto* external_ptr = new dummy_external_interface(sys);
    initialize_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

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

TEST_CASE("animation_timer", "[timing][ticks]")
{
    alia::system sys;
    auto* external_ptr = new dummy_external_interface(sys);
    initialize_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

    REQUIRE(!system_needs_refresh(sys));

    do_traversal(sys, [&](context ctx) {
        animation_timer timer(ctx);
        REQUIRE(!timer.is_active());
        auto start = actions::start(timer) << 100;
        REQUIRE(action_is_ready(start));
        perform_action(start);
    });

    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 10;

    do_traversal(sys, [&](context ctx) {
        animation_timer_state* state;
        get_cached_data(ctx, &state);
        animation_timer timer(ctx, *state);
        REQUIRE(timer.is_active());
        REQUIRE(timer.ticks_left() == 90);
    });

    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 110;

    do_traversal(sys, [&](context ctx) {
        animation_timer timer(ctx);
        REQUIRE(!timer.is_active());
    });

    REQUIRE(!system_needs_refresh(sys));
}
