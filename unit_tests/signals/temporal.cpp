#include <alia/signals/temporal.hpp>

#include <catch.hpp>

#include <thread>

#include "traversal.hpp"

using namespace alia;

struct dummy_external_interface : external_interface
{
    bool refresh_requested = false;

    void
    request_animation_refresh()
    {
        refresh_requested = true;
    }
};

TEST_CASE("get_animation_ticks_left", "[signals][temporal]")
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
