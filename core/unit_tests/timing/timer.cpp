#include <alia/core/timing/timer.hpp>

#include <alia/core/actions/operators.hpp>

#include <catch2/catch_test_macros.hpp>

#include "traversal.hpp"

using namespace alia;

struct testing_external_interface : default_external_interface
{
    testing_external_interface(alia::system& sys)
        : default_external_interface(sys)
    {
    }

    millisecond_count tick_count = 0;

    millisecond_count
    get_tick_count() const
    {
        return tick_count;
    }
};

TEST_CASE("timer", "[timing][timer]")
{
    alia::system sys;
    auto* external_ptr = new testing_external_interface(sys);
    initialize_standalone_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(!timer.is_active());
        REQUIRE(!timer.is_triggered());
        timer.start(100);
    });

    auto count_timer_events = [&]() {
        int event_count = 0;
        sys.controller = [&](context ctx) {
            timer timer(ctx);
            if (timer.is_triggered())
                ++event_count;
        };
        process_internal_callbacks(sys, external.tick_count);
        return event_count;
    };

    external.tick_count = 10;
    REQUIRE(count_timer_events() == 0);

    external.tick_count = 110;
    REQUIRE(count_timer_events() == 1);

    external.tick_count = 120;
    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(!timer.is_active());
        REQUIRE(!timer.is_triggered());
        timer.start(50);
    });

    external.tick_count = 130;
    do_traversal(sys, [&](context ctx) {
        timer_data* data;
        get_cached_data(ctx, &data);
        timer timer(ctx, *data);
        REQUIRE(timer.is_active());
        REQUIRE(!timer.is_triggered());
    });

    external.tick_count = 150;
    REQUIRE(count_timer_events() == 0);

    external.tick_count = 170;
    REQUIRE(count_timer_events() == 1);

    external.tick_count = 200;
    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(!timer.is_active());
        REQUIRE(!timer.is_triggered());
        timer.start(100);
    });

    external.tick_count = 210;
    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(timer.is_active());
        REQUIRE(!timer.is_triggered());
        perform_action(actions::stop(timer));
    });

    external.tick_count = 220;
    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(!timer.is_active());
        REQUIRE(!timer.is_triggered());
    });

    external.tick_count = 320;
    REQUIRE(count_timer_events() == 0);

    external.tick_count = 350;
    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(!timer.is_active());
        REQUIRE(!timer.is_triggered());
        perform_action(actions::start(timer) << 100);
    });

    auto count_and_restart = [&]() {
        int event_count = 0;
        sys.controller = [&](context ctx) {
            timer timer(ctx);
            if (timer.is_triggered())
            {
                ++event_count;
                timer.start(100);
            }
        };
        process_internal_callbacks(sys, external.tick_count);
        return event_count;
    };

    external.tick_count = 400;
    REQUIRE(count_and_restart() == 0);

    external.tick_count = 500;
    REQUIRE(count_and_restart() == 1);

    external.tick_count = 520;
    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(timer.is_active());
        REQUIRE(!timer.is_triggered());
    });

    // By 550, we should get another event because the restart should've been
    // calculated to have happened at 450.
    external.tick_count = 550;
    REQUIRE(count_timer_events() == 1);

    external.tick_count = 555;
    do_traversal(sys, [&](context ctx) {
        timer timer(ctx);
        REQUIRE(!timer.is_active());
        REQUIRE(!timer.is_triggered());
    });
}
