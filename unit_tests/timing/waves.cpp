#include <alia/timing/waves.hpp>

#include <testing.hpp>

#include <alia/signals/lambdas.hpp>

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

TEST_CASE("square_wave", "[timing][waves]")
{
    alia::system sys;
    auto* external_ptr = new testing_external_interface(sys);
    initialize_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

    std::function<value_signal<bool>(context)> square_wave_invoker
        = [&](context ctx) { return square_wave(ctx, 50); };

    // Define a utility function for processing timing events.
    auto process_timing_events = [&]() {
        sys.controller = [&](context ctx) { square_wave_invoker(ctx); };
        process_internal_timing_events(sys, external.tick_count);
    };

    // Our square wave should start out at true.
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });

    // ... and it should stay that way for a while.
    external.tick_count = 40;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });

    // After 50 milliseconds, it should switch to false.
    external.tick_count = 50;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });

    // ... and it should stay that way for a while.
    external.tick_count = 90;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });

    // ... and then it should switch back again to true.
    external.tick_count = 100;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });

    // It should respond properly if we give it asymmetric durations...
    square_wave_invoker = [&](context ctx) { return square_wave(ctx, 50, 10); };
    external.tick_count = 140;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });
    external.tick_count = 150;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });
    external.tick_count = 155;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });
    external.tick_count = 160;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });
    external.tick_count = 200;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });
    external.tick_count = 210;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });
    external.tick_count = 215;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });

    // If we take away the durations, it should stop alternating...
    square_wave_invoker = [&](context ctx) {
        return square_wave(ctx, empty<millisecond_count>());
    };
    for (external.tick_count = 220; external.tick_count < 400;
         external.tick_count += 10)
    {
        process_timing_events();
        do_traversal(sys, [&](context ctx) {
            auto x = square_wave_invoker(ctx);
            REQUIRE(signal_has_value(x));
            REQUIRE(read_signal(x) == true);
        });
    }

    // And once we give it durations again, it picks them up...
    square_wave_invoker = [&](context ctx) { return square_wave(ctx, 20); };
    external.tick_count = 400;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });
    external.tick_count = 410;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });
    external.tick_count = 420;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });
    external.tick_count = 430;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == false);
    });
    external.tick_count = 440;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });
    external.tick_count = 450;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = square_wave_invoker(ctx);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == true);
    });
}
