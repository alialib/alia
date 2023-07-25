#include <alia/core/timing/deflicker.hpp>

#include <catch2/catch_test_macros.hpp>

#include <alia/core/signals/lambdas.hpp>

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

TEST_CASE("deflicker", "[timing][deflicker]")
{
    alia::system sys;
    auto* external_ptr = new testing_external_interface(sys);
    initialize_standalone_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

    int x_value = 1;
    bool x_valid = true;

    // Define a utility function for processing timing events.
    auto process_timing_events = [&]() {
        sys.controller = [&](context ctx) {
            auto x = lambda_reader(
                [&]() { return x_valid; }, [&]() { return x_value; });
            auto d = deflicker(ctx, x, 50);
        };
        process_internal_timing_events(sys, external.tick_count);
    };

    // Our deflickered signal should pick up its initial value.
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 1);
        auto d = deflicker(ctx, x, 50);
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 1);
    });

    // Our deflickered signal should pick up a value change.
    // (Also try conveying the delay as a signal.)
    external.tick_count = 10;
    process_timing_events();
    x_value = 2;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 2);
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });

    // Our deflickered signal should maintain its value when :x loses its
    // value.
    external.tick_count = 20;
    process_timing_events();
    x_valid = false;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(!signal_has_value(x));
        auto d = deflicker(ctx, x, 50);
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });

    // ... and continue to do so.
    external.tick_count = 60;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(!signal_has_value(x));
        auto d = deflicker(ctx, x, 50);
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });

    // After its delay runs out, our deflickered signal should lose its value.
    external.tick_count = 70;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(!signal_has_value(x));
        auto d = deflicker(ctx, x, 50);
        REQUIRE(!signal_has_value(d));
    });

    // When x regains its value, so should the deflickered signal.
    external.tick_count = 80;
    process_timing_events();
    x_value = 1;
    x_valid = true;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 1);
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 1);
    });

    // If x loses its value and there's no delay signal, the deflickered signal
    // should immediately lose its value.
    external.tick_count = 90;
    process_timing_events();
    x_valid = false;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(!signal_has_value(x));
        auto d = deflicker(ctx, x, empty<millisecond_count>());
        REQUIRE(!signal_has_value(d));
    });

    // When x regains its value again, so should the deflickered signal.
    external.tick_count = 100;
    process_timing_events();
    x_value = 1;
    x_valid = true;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 1);
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 1);
    });

    // Take away x's value again.
    external.tick_count = 110;
    process_timing_events();
    x_valid = false;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(!signal_has_value(x));
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 1);
    });

    // If x regains its value before the delay runs out, the deflickered signal
    // should pick up that new value.
    external.tick_count = 120;
    process_timing_events();
    x_value = 2;
    x_valid = true;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 2);
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });

    // ... and it should maintain the new value even after the delay runs out.
    external.tick_count = 200;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 2);
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });

    // The deflickered signal shouldn't be confused by a rapid succession of
    // changes in x...
    external.tick_count = 200;
    process_timing_events();
    x_value = 1;
    x_valid = true;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 1);
    });
    external.tick_count = 210;
    process_timing_events();
    x_valid = false;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 1);
    });
    external.tick_count = 220;
    process_timing_events();
    x_value = 2;
    x_valid = true;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });
    external.tick_count = 230;
    process_timing_events();
    x_valid = false;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });
    external.tick_count = 270;
    process_timing_events();
    x_valid = false;
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(signal_has_value(d));
        REQUIRE(read_signal(d) == 2);
    });
    external.tick_count = 290;
    process_timing_events();
    do_traversal(sys, [&](context ctx) {
        auto x = lambda_reader(
            [&]() { return x_valid; }, [&]() { return x_value; });
        auto d = deflicker(ctx, x, value(50));
        REQUIRE(!signal_has_value(d));
    });
}

TEST_CASE("initially empty deflicker", "[timing][deflicker]")
{
    alia::system sys;
    auto* external_ptr = new testing_external_interface(sys);
    initialize_standalone_system(
        sys, [](context) {}, external_ptr);

    // Test that our deflickered signal picks up an initially empty value.
    do_traversal(sys, [&](context ctx) {
        auto d = deflicker(ctx, empty<int>(), 50);
        REQUIRE(!signal_has_value(d));
    });
}
