#include <alia/signals/temporal.hpp>

#include <catch.hpp>

#include <complex>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("animation_timer", "[signals][temporal]")
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

TEST_CASE("interpolation", "[signals][temporal]")
{
    REQUIRE(interpolate(-1., 1., 0.6) == Approx(0.2));
    REQUIRE(interpolate(0, 10, 0.57) == 6);
    REQUIRE(interpolate(0, 10, 0.64) == 6);
    REQUIRE(interpolate(1.f, 3.f, 0.5) == Approx(2.f));
    REQUIRE(
        interpolate(std::complex<double>(0), std::complex<double>(4), 0.25)
            .real()
        == Approx(1));
}

TEST_CASE("smooth_raw_value", "[signals][temporal]")
{
    alia::system sys;

    set_automatic_time_updates(sys, false);
    set_millisecond_tick_counter(sys, 0);

    REQUIRE(!system_needs_refresh(sys));

    value_smoother<int> smoother;

    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(!system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 100);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 10, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 110);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 10, animated_transition{linear_curve, 100});
        REQUIRE(x == 1);
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 150);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 5);
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 170);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 3);
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 200);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(!system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 300);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 20, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 350);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 30, animated_transition{linear_curve, 100});
        REQUIRE(x == 10);
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 400);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 30, animated_transition{linear_curve, 100});
        REQUIRE(x == 20);
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 450);
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw_value(
            ctx, smoother, 30, animated_transition{linear_curve, 100});
        REQUIRE(x == 30);
    });
    REQUIRE(!system_needs_refresh(sys));
}

TEST_CASE("smooth_value", "[signals][temporal]")
{
    alia::system sys;

    set_automatic_time_updates(sys, false);
    set_millisecond_tick_counter(sys, 0);

    REQUIRE(!system_needs_refresh(sys));

    auto transition = animated_transition{linear_curve, 100};

    do_traversal(sys, [&](context ctx) {
        auto x = smooth_value(ctx, value(0), transition);
        REQUIRE(signal_is_readable(x));
        REQUIRE(read_signal(x) == 0);
    });
    REQUIRE(!system_needs_refresh(sys));

    captured_id last_id;

    set_millisecond_tick_counter(sys, 100);
    do_traversal(sys, [&](context ctx) {
        auto x = smooth_value(ctx, value(10), transition);
        REQUIRE(signal_is_readable(x));
        REQUIRE(read_signal(x) == 0);
        last_id.capture(x.value_id());
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 150);
    do_traversal(sys, [&](context ctx) {
        auto x = smooth_value(ctx, value(10), transition);
        REQUIRE(signal_is_readable(x));
        REQUIRE(read_signal(x) == 5);
        REQUIRE(last_id != x.value_id());
        last_id.capture(x.value_id());
    });
    REQUIRE(system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 200);
    do_traversal(sys, [&](context ctx) {
        auto x = smooth_value(ctx, value(10), transition);
        REQUIRE(signal_is_readable(x));
        REQUIRE(read_signal(x) == 10);
        REQUIRE(last_id != x.value_id());
        last_id.capture(x.value_id());
    });
    REQUIRE(!system_needs_refresh(sys));

    set_millisecond_tick_counter(sys, 240);
    do_traversal(sys, [&](context ctx) {
        auto x = smooth_value(ctx, empty<int>(), transition);
        REQUIRE(!signal_is_readable(x));
        REQUIRE(last_id != x.value_id());
    });
    REQUIRE(!system_needs_refresh(sys));
}
